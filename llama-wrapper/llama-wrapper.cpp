#include "llama.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "llama-wrapper.h"

typedef struct {
    llama_batch batch;
    const llama_vocab* vocab;
    llama_sampler* smpl;
    int n_pos;
    int n_predict;
} llama_wrapper_context;

llama_wrapper_context lwc;

void batch_clear(struct llama_batch &batch)
{
    batch.n_tokens = 0;
}

void batch_add(struct llama_batch &batch, llama_token id, llama_pos pos, const llama_seq_id seq_ids[], const int n_seq_ids, bool logits)
{
    batch.token[batch.n_tokens] = id;
    batch.pos[batch.n_tokens] = pos;
    batch.n_seq_id[batch.n_tokens] = n_seq_ids;
    for (size_t i = 0; i < n_seq_ids; i++) {
        batch.seq_id[batch.n_tokens][i] = seq_ids[i];
    }
    batch.logits[batch.n_tokens] = logits;
    batch.n_tokens++;
}

extern "C" int llama_wrapper_completion_init(llama_model* model, llama_context* ctx, const char* prompt, int n_predict)
{
    if (!model || !ctx || !prompt) return 1;

    if (lwc.vocab == NULL) {
        lwc.vocab = llama_model_get_vocab(model);
    }

    lwc.n_predict = n_predict;

    int n_prompt = -llama_tokenize(lwc.vocab, prompt, (int32_t)strlen(prompt), NULL, 0, true, true);
    if (n_prompt <= 0) return 1;

    llama_token* tokens = (llama_token*)malloc(sizeof(llama_token) * n_prompt);
    if (llama_tokenize(lwc.vocab, prompt, (int32_t)strlen(prompt), tokens, n_prompt, true, true) < 0) {
        free(tokens);
        return 1;
    }

    if (lwc.smpl == NULL) {
        llama_sampler_chain_params sparams = llama_sampler_chain_default_params();
        lwc.smpl = llama_sampler_chain_init(sparams);
        llama_sampler_chain_add(lwc.smpl, llama_sampler_init_temp(0.8f));
        llama_sampler_chain_add(lwc.smpl, llama_sampler_init_penalties(64, 1.1f, 0.0f, 0.0f));
        llama_sampler_chain_add(lwc.smpl, llama_sampler_init_top_k(40));
        llama_sampler_chain_add(lwc.smpl, llama_sampler_init_top_p(0.9f, 5));
        llama_sampler_chain_add(lwc.smpl, llama_sampler_init_greedy());
    }

  
    const int n_parallel = 1;
    lwc.batch = llama_batch_init(n_prompt, 0, n_parallel);
    
    llama_seq_id seq_ids = 0;
    batch_clear(lwc.batch);
    
    for (int i = 0; i < n_prompt; i++) {
        batch_add(lwc.batch, tokens[i], i, &seq_ids, 1, false);
    }
    
    lwc.batch.logits[lwc.batch.n_tokens - 1] = true;
    
    if (llama_decode(ctx, lwc.batch)) return 1;
    lwc.n_pos = lwc.batch.n_tokens;
    
    free(tokens);
    
    return 0;
}

extern "C" int llama_wrapper_completion_loop(llama_model* model, llama_context* ctx, char* result, int n_result)
{
    if (!model || !ctx) return 1;

    if (lwc.n_pos >= lwc.n_predict) {
        return 1;
    }
      
    llama_seq_id seq_ids = 0;
    size_t len = 0;

    llama_token new_token_id = llama_sampler_sample(lwc.smpl, ctx, -1);
    if (llama_vocab_is_eog(lwc.vocab, new_token_id)) return 1;

    char piece[128];
    int n = llama_token_to_piece(lwc.vocab, new_token_id, piece, sizeof(piece), 0, true);
    if (n > 0) {
        memcpy(result + len, piece, n);
        len += n;
    }
        
    batch_clear(lwc.batch);
   
    batch_add(lwc.batch, new_token_id, lwc.n_pos, &seq_ids, 1, true);

    if (llama_decode(ctx, lwc.batch)) return 1;

    lwc.n_pos += lwc.batch.n_tokens;
    
    result[len] = 0;
    
    return 0;
}

extern "C" void llama_wrapper_completion_clear(llama_context* context) {
    llama_memory_clear(llama_get_memory(context), true);
}

extern "C" llama_model* llama_wrapper_init_model(const char* model_path, int n_gpu_layers) {
    llama_model_params params = llama_model_default_params();
    params.n_gpu_layers = n_gpu_layers;
    
    memset(&lwc, 0, sizeof(lwc));
    return llama_model_load_from_file(model_path, params);
}

extern "C" void llama_wrapper_free_model(llama_model* model) {
    if (model) llama_model_free(model);
}

extern "C" llama_context* llama_wrapper_init_context(llama_model* model, int n_ctx, int n_batch) {
    llama_context_params ctx_params = llama_context_default_params();
    ctx_params.n_ctx = n_ctx;
    ctx_params.n_batch = n_batch;
    return llama_init_from_model(model, ctx_params);
}

extern "C" void llama_wrapper_free_context(llama_context* ctx) {
    if (ctx) llama_free(ctx);
}

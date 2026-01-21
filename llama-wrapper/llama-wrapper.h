// MyWrapper.h
#include "llama.h"

#ifdef __cplusplus
extern "C" {
#endif

int llama_wrapper_completion(struct llama_model* model, struct llama_context* ctx, const char* prompt, int n_predict, char* result, int n_result);
int llama_wrapper_completion_init(struct llama_model* model, struct llama_context* ctx, const char* prompt, int n_predict);
int llama_wrapper_completion_loop(struct llama_model* model, struct llama_context* ctx, char* result, int n_result);
void llama_wrapper_completion_clear(struct llama_context* context);

struct llama_model* llama_wrapper_init_model(const char* model_path, int n_gpu_layers);
void llama_wrapper_free_model(struct llama_model* model);
struct llama_context* llama_wrapper_init_context(struct llama_model* model, int n_ctx, int n_batch);
void llama_wrapper_free_context(struct llama_context* ctx);

#ifdef __cplusplus
}
#endif

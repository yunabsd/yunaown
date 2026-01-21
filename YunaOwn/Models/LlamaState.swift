import Foundation
import Combine

@MainActor
class ChatMessage: Identifiable {
    let id = UUID()
    @Published var text: String
    let isUser: Bool
    
    init(text: String, isUser: Bool) {
        self.text = text
        self.isUser = isUser
    }
}

@MainActor
class LlamaState: ObservableObject {
    private var model: OpaquePointer
    private var context: OpaquePointer
    @Published var isDone: Bool = false
    @Published var messages: [ChatMessage] = []
    
    init() {
        guard let modelPath = Bundle.main.path(forResource: "gemma-3-4b-it-q4_k_m", ofType: "gguf")
            else {
                fatalError("model not found")
            }
        
        var nGpuLayers: Int32 = 99
#if targetEnvironment(simulator)
        nGpuLayers = 0
#endif
      
        self.model = llama_wrapper_init_model(modelPath, nGpuLayers)
        self.context = llama_wrapper_init_context(self.model, 1024, 128)
        self.isDone = true
    }
    
    deinit {
        llama_wrapper_free_model(model)
        llama_wrapper_free_context(context)
    }
    
    func completion(prompt: String) {
        var buf = [CChar](repeating: 0, count: 8192)
        let promptTemplate = """
        <start_of_turn>user
        \(prompt)
        <end_of_turn>
        <start_of_turn>model
        """

        messages.append(ChatMessage(text: prompt, isUser: true))
        
        self.isDone = false;
        
        llama_wrapper_completion_init(self.model, self.context, promptTemplate, 512)

        messages.append(ChatMessage(text: "", isUser: false))
        var m = ""
        
        var isFirst = true;
        
        Task.detached {
            while true {
                if await llama_wrapper_completion_loop(self.model, self.context, &buf, Int32(buf.count)) == 0 {
                    await MainActor.run {
                        m += String(cString: buf)
                        if isFirst {
                            m = m.trimmingCharacters(in: .whitespacesAndNewlines)
                            isFirst = false
                        }
                        self.messages[self.messages.count - 1] = ChatMessage(text: m, isUser: false)
                    }
                }
                else {
                    // finished
                    await MainActor.run {
                        self.isDone = true
                    }
                    break
                }
                
            }

            await MainActor.run {
                llama_wrapper_completion_clear(self.context)
            }
        }
    }
}

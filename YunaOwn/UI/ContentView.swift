//
//  ContentView.swift
//  ai
//
//  Created by Yuna Seo on 1/20/26.
//

import SwiftUI

struct ContentView: View {
    @StateObject var llamaState = LlamaState()
    @State private var inputText: String = ""
    @FocusState private var isInputFocused: Bool
    
    var body: some View {
        ZStack {
            Color(.black)
            .ignoresSafeArea()
            .onTapGesture {
                isInputFocused = false
            }
            VStack {
                ScrollViewReader { scrollView in
                    ScrollView {
                        VStack(spacing: 8) {
                            ForEach(llamaState.messages) { message in
                                HStack {
                                    if message.isUser { Spacer() }
                                    Text(message.text)
                                        .padding(10)
                                        .background(message.isUser ? Color.blue : Color(hex: 0x222222))
                                        .foregroundColor(message.isUser ? Color.white : Color.white)
                                        .cornerRadius(12)
                                        .id(message.id)
                                    if !message.isUser { Spacer() }
                                }
                                .padding(.horizontal)
                            }
                        }
                        .onChange(of: llamaState.messages.last?.text) { _ in
                            if let last = llamaState.messages.last {
                                withAnimation {
                                    scrollView.scrollTo(last.id, anchor: .bottom)
                                }
                            }
                        }
                    }
                    .simultaneousGesture(
                        TapGesture()
                            .onEnded { _ in
                                isInputFocused = false
                            }
                    )
                }
                
                Divider()
                
                HStack {
                    TextField("메시지를 입력해주세요...", text: $inputText)
                        .cornerRadius(8)
                        .focused($isInputFocused)
                    
                    Button(action: sendMessage) {
                        Text("전송")
                            .bold()
                            .padding(.vertical, 8)
                            .padding(.horizontal, 16)
                            .background(Color.blue)
                            .foregroundColor(.white)
                            .cornerRadius(8)
                    }
                    .disabled(isDisabled)
                    .opacity(isDisabled ? 0.3 : 1.0)
                }
                .padding()
            }
        }
    }
    
    private func sendMessage() {
        guard !inputText.trimmingCharacters(in: .whitespaces).isEmpty else { return }
       
        llamaState.completion(prompt: inputText)

        inputText = ""
    }
    
    private var isDisabled: Bool {
        return inputText.isEmpty || llamaState.isDone != true
    }
}

extension Color {
    init(hex: UInt, alpha: Double = 1.0) {
        self.init(
            .sRGB,
            red: Double((hex >> 16) & 0xFF) / 255,
            green: Double((hex >> 8) & 0xFF) / 255,
            blue: Double(hex & 0xFF) / 255,
            opacity: alpha
        )
    }
}

#Preview {
    ContentView()
}

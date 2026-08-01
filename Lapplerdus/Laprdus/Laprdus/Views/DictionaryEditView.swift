// DictionaryEditView.swift - Add/edit a single dictionary entry.

import SwiftUI

struct DictionaryEditView: View {
    let entry: DictionaryEntry?
    let onSave: (DictionaryEntry) -> Void
    let onDelete: (DictionaryEntry) -> Void
    let onDuplicate: (DictionaryEntry) -> Void

    @Environment(\.dismiss) private var dismiss

    @State private var grapheme = ""
    @State private var phoneme = ""
    @State private var comment = ""
    @State private var caseSensitive = false
    @State private var wholeWord = true
    @State private var graphemeMissing = false
    @State private var phonemeMissing = false
    @State private var showDeleteConfirmation = false

    private var isEditing: Bool { entry != nil }

    var body: some View {
        Form {
            Section {
                TextField("Original text", text: $grapheme)
                    .autocorrectionDisabled()
                if graphemeMissing {
                    Text("This field is required")
                        .font(.footnote)
                        .foregroundStyle(.red)
                }
                TextField("Replacement pronunciation", text: $phoneme)
                    .autocorrectionDisabled()
                if phonemeMissing {
                    Text("This field is required")
                        .font(.footnote)
                        .foregroundStyle(.red)
                }
                TextField("Comment (optional)", text: $comment, axis: .vertical)
                    .lineLimit(1...3)
            }

            Section {
                Toggle("Case sensitive", isOn: $caseSensitive)
                Toggle("Match whole word only", isOn: $wholeWord)
            }

            Section {
                Button {
                    save()
                } label: {
                    Text("Save")
                        .frame(maxWidth: .infinity)
                        .fontWeight(.semibold)
                }
            }
        }
        .formStyle(.grouped)
        .navigationTitle(isEditing ? Text("Edit Entry") : Text("Add Entry"))
        #if !os(macOS)
        .navigationBarTitleDisplayMode(.inline)
        #endif
        .toolbar {
            if let entry {
                ToolbarItem {
                    Button {
                        onDuplicate(entry)
                        dismiss()
                    } label: {
                        Label("Duplicate", systemImage: "doc.on.doc")
                    }
                }
                ToolbarItem {
                    Button(role: .destructive) {
                        showDeleteConfirmation = true
                    } label: {
                        Label("Delete", systemImage: "trash")
                    }
                }
            }
        }
        .alert(Text("Delete entry?"), isPresented: $showDeleteConfirmation) {
            Button("Delete", role: .destructive) {
                if let entry {
                    onDelete(entry)
                }
                dismiss()
            }
            Button("Cancel", role: .cancel) {}
        } message: {
            Text("Are you sure you want to delete this dictionary entry?")
        }
        .onAppear {
            if let entry {
                grapheme = entry.grapheme
                phoneme = entry.phoneme
                comment = entry.comment
                caseSensitive = entry.caseSensitive
                wholeWord = entry.wholeWord
            }
        }
        .onChange(of: grapheme) { _, _ in graphemeMissing = false }
        .onChange(of: phoneme) { _, _ in phonemeMissing = false }
    }

    private func save() {
        let trimmedGrapheme = grapheme.trimmingCharacters(in: .whitespacesAndNewlines)
        let trimmedPhoneme = phoneme.trimmingCharacters(in: .whitespacesAndNewlines)
        graphemeMissing = trimmedGrapheme.isEmpty
        phonemeMissing = trimmedPhoneme.isEmpty
        guard !graphemeMissing, !phonemeMissing else { return }

        onSave(DictionaryEntry(
            id: entry?.id ?? UUID(),
            grapheme: trimmedGrapheme,
            phoneme: trimmedPhoneme,
            caseSensitive: caseSensitive,
            wholeWord: wholeWord,
            comment: comment.trimmingCharacters(in: .whitespacesAndNewlines)
        ))
        dismiss()
    }
}

#Preview {
    NavigationStack {
        DictionaryEditView(entry: nil, onSave: { _ in }, onDelete: { _ in }, onDuplicate: { _ in })
    }
}

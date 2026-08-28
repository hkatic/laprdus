// DictionaryEditView.swift - Modal form for adding or editing a dictionary entry.

import SwiftUI

struct DictionaryEditView: View {
    let entry: DictionaryEntry?
    let onSave: (DictionaryEntry) -> Void
    let onDelete: (DictionaryEntry) -> Void

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

    /// True when the form differs from what it started with; used to block
    /// the accidental swipe-down dismissal that would silently discard input.
    private var hasUnsavedChanges: Bool {
        if let entry {
            return grapheme != entry.grapheme
                || phoneme != entry.phoneme
                || comment != entry.comment
                || caseSensitive != entry.caseSensitive
                || wholeWord != entry.wholeWord
        }
        return !grapheme.isEmpty || !phoneme.isEmpty || !comment.isEmpty
    }

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

            if isEditing {
                Section {
                    Button(role: .destructive) {
                        showDeleteConfirmation = true
                    } label: {
                        Text("Delete Entry")
                            .frame(maxWidth: .infinity)
                    }
                }
            }
        }
        .formStyle(.grouped)
        .navigationTitle(isEditing ? Text("Edit Entry") : Text("Add Entry"))
        .inlineNavigationBarTitle()
        .interactiveDismissDisabled(hasUnsavedChanges)
        .toolbar {
            ToolbarItem(placement: .cancellationAction) {
                Button("Cancel") {
                    dismiss()
                }
            }
            ToolbarItem(placement: .confirmationAction) {
                Button("Save") {
                    save()
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
        .onChange(of: grapheme) { _ in graphemeMissing = false }
        .onChange(of: phoneme) { _ in phonemeMissing = false }
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
        DictionaryEditView(entry: nil, onSave: { _ in }, onDelete: { _ in })
    }
}

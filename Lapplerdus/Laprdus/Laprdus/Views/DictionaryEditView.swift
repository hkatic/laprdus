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
                LabeledField(label: String(localized: "Original text"), isMissing: graphemeMissing) {
                    TextField("", text: $grapheme)
                        .autocorrectionDisabled()
                        .accessibilityLabel(Text("Original text"))
                }
                LabeledField(label: String(localized: "Replacement pronunciation"), isMissing: phonemeMissing) {
                    TextField("", text: $phoneme)
                        .autocorrectionDisabled()
                        .accessibilityLabel(Text("Replacement pronunciation"))
                }
                LabeledField(label: String(localized: "Comment (optional)"), isMissing: false) {
                    TextField("", text: $comment, axis: .vertical)
                        .lineLimit(1...3)
                        .accessibilityLabel(Text("Comment (optional)"))
                }
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

/// A text field that always shows what it is for. A plain placeholder would
/// vanish as soon as the field has content, leaving an entry being edited as
/// unlabeled boxes.
///
/// The two platforms want different shapes for this. A macOS grouped Form
/// splits every row into a label column and a trailing control column, so it
/// gets a real LabeledContent row; fighting that layout leaves the field
/// stranded on the right with its text jammed against the bezel. iOS has no
/// such column, so the name sits above the field as a caption. Either way the
/// name is hidden from VoiceOver, which reads it from the field itself.
private struct LabeledField<Content: View>: View {
    let label: String
    let isMissing: Bool
    @ViewBuilder var content: Content

    var body: some View {
        #if os(macOS)
        LabeledContent {
            VStack(alignment: .leading, spacing: 2) {
                content
                    .textFieldStyle(.roundedBorder)
                    .multilineTextAlignment(.leading)
                requiredNote
            }
        } label: {
            Text(label)
                .foregroundStyle(isMissing ? Color.red : Color.primary)
                .accessibilityHidden(true)
        }
        #else
        VStack(alignment: .leading, spacing: 2) {
            Text(label)
                .font(.caption)
                .foregroundStyle(isMissing ? Color.red : Color.secondary)
                .accessibilityHidden(true)
            content
            requiredNote
        }
        .padding(.vertical, 2)
        #endif
    }

    @ViewBuilder
    private var requiredNote: some View {
        if isMissing {
            Text("This field is required")
                .font(.footnote)
                .foregroundStyle(.red)
        }
    }
}

#Preview {
    NavigationStack {
        DictionaryEditView(entry: nil, onSave: { _ in }, onDelete: { _ in })
    }
}

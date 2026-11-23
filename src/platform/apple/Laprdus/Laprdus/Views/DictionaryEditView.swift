// DictionaryEditView.swift - Modal form for adding or editing a dictionary
// entry. The accent lexicon has a form of its own: the entry in the engine's
// notation, its kind (word or verb), its language and a comment; the
// engine's parser checks the entry before it is saved.

import SwiftUI

struct DictionaryEditView: View {
    let type: DictionaryType
    let entry: DictionaryEntry?
    let onSave: (DictionaryEntry) -> Void
    let onDelete: (DictionaryEntry) -> Void

    init(type: DictionaryType = .main,
         entry: DictionaryEntry?,
         onSave: @escaping (DictionaryEntry) -> Void,
         onDelete: @escaping (DictionaryEntry) -> Void) {
        self.type = type
        self.entry = entry
        self.onSave = onSave
        self.onDelete = onDelete
    }

    @Environment(\.dismiss) private var dismiss

    @State private var grapheme = ""
    @State private var phoneme = ""
    @State private var comment = ""
    @State private var caseSensitive = false
    @State private var wholeWord = true
    @State private var isVerb = false
    @State private var language = ""
    @State private var graphemeMissing = false
    @State private var phonemeMissing = false
    /// The engine's reason for refusing an accent entry, or nil.
    @State private var accentError: String?
    @State private var showDeleteConfirmation = false

    private var isEditing: Bool { entry != nil }
    private var isAccents: Bool { type == .accents }

    /// True when the form differs from what it started with; used to block
    /// the accidental swipe-down dismissal that would silently discard input.
    private var hasUnsavedChanges: Bool {
        if let entry {
            return grapheme != entry.grapheme
                || phoneme != entry.phoneme
                || comment != entry.comment
                || caseSensitive != entry.caseSensitive
                || wholeWord != entry.wholeWord
                || isVerb != entry.isVerb
                || language != entry.language
        }
        return !grapheme.isEmpty || !phoneme.isEmpty || !comment.isEmpty
            || isVerb || !language.isEmpty
    }

    var body: some View {
        Form {
            if isAccents {
                accentSections
            } else {
                replacementSections
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
                isVerb = entry.isVerb
                language = entry.language
            }
        }
        .onChange(of: grapheme) { _ in
            graphemeMissing = false
            accentError = nil
        }
        .onChange(of: phoneme) { _ in phonemeMissing = false }
        .onChange(of: isVerb) { _ in accentError = nil }
    }

    /// The fields of a replacement entry (main, spelling and emoji dictionaries).
    @ViewBuilder
    private var replacementSections: some View {
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
    }

    /// The fields of an accent lexicon entry: the entry text, its kind (word
    /// or verb), its language and a comment, with a reminder of the notation.
    @ViewBuilder
    private var accentSections: some View {
        Section {
            LabeledField(label: String(localized: "Entry (a word, stem, list of forms or verb with its accent)"),
                         isMissing: graphemeMissing, note: accentNote) {
                TextField("", text: $grapheme)
                    .autocorrectionDisabled()
                    #if os(iOS)
                    .textInputAutocapitalization(.never)
                    #endif
                    .accessibilityLabel(Text("Entry (a word, stem, list of forms or verb with its accent)"))
            }
            Picker("Kind of entry", selection: $isVerb) {
                Text("Word").tag(false)
                Text("Verb").tag(true)
            }
            Picker("Language", selection: $language) {
                Text("All languages").tag("")
                Text("Croatian").tag("hr")
                Text("Serbian").tag("sr")
                Text("Bosnian").tag("bs")
                Text("Slovenian").tag("sl")
            }
            LabeledField(label: String(localized: "Comment (optional)"), isMissing: false) {
                TextField("", text: $comment, axis: .vertical)
                    .lineLimit(1...3)
                    .accessibilityLabel(Text("Comment (optional)"))
            }
        }

        Section {
            Text("Marks: ' before the stressed vowel (the tone is chosen for you), ^ falling accent, / rising accent, : after a long vowel, * at the end of a stem (kontr'o:l* covers kontróla, kontróle, kontrólom), | between forms (sign'a:l|a|u|om). Verb: the whole stem, = and the class (ur'e:d=i<p for uréditi, urédim), or a root with a long ije (dijel:i for podijeliti).")
                .font(.footnote)
                .foregroundStyle(.secondary)
        }
    }

    /// What the engine objected to in the entry, shown under the field.
    private var accentNote: String? {
        guard let accentError else { return nil }
        return String(localized: "The entry is not valid: \(accentError)")
    }

    private func save() {
        if isAccents {
            saveAccentEntry()
            return
        }
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

    private func saveAccentEntry() {
        let text = grapheme.trimmingCharacters(in: .whitespacesAndNewlines)
        graphemeMissing = text.isEmpty
        guard !graphemeMissing else { return }

        let newEntry = DictionaryEntry(
            id: entry?.id ?? UUID(),
            grapheme: text,
            comment: comment.trimmingCharacters(in: .whitespacesAndNewlines),
            isVerb: isVerb,
            language: language
        )
        // The engine's own parser decides whether the entry is well formed,
        // so a typo is caught here and not silently skipped at load time.
        if let reason = LaprdusEngine.checkAccentLexicon(DictionaryStore.accentLexicon(of: newEntry)) {
            accentError = reason
            return
        }
        onSave(newEntry)
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
    /// A problem with the field's content other than being empty, shown
    /// under it in the same way.
    var note: String? = nil
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
        } else if let note {
            Text(note)
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

// DictionaryListView.swift - Dictionaries tab: dictionary selection and
// entry management (main/spelling/emoji/accents).

import SwiftUI

struct DictionaryListView: View {
    @EnvironmentObject private var model: AppModel

    @State private var selectedType: DictionaryType = .main
    @State private var entries: [DictionaryEntry] = []
    @State private var isLoading = true
    @State private var errorMessage: String?
    @State private var editorRoute: EditorRoute?
    @State private var entryPendingDeletion: DictionaryEntry?

    private struct EditorRoute: Identifiable {
        let id = UUID()
        var entry: DictionaryEntry?
    }

    var body: some View {
        List {
            Section {
                Picker("Dictionary type", selection: $selectedType) {
                    Text("Main Dictionary").tag(DictionaryType.main)
                    Text("Spelling Dictionary").tag(DictionaryType.spelling)
                    Text("Emoji Dictionary").tag(DictionaryType.emoji)
                    Text("Accent Dictionary").tag(DictionaryType.accents)
                }
                .accessibilityHint(Text("Select dictionary type"))
            }

            Section {
                if isLoading {
                    ProgressView()
                        .frame(maxWidth: .infinity)
                } else if entries.isEmpty {
                    Text("No entries yet. Tap + to add one.")
                        .foregroundStyle(.secondary)
                        .frame(maxWidth: .infinity, alignment: .center)
                } else {
                    ForEach(entries) { entry in
                        Button {
                            editorRoute = EditorRoute(entry: entry)
                        } label: {
                            HStack {
                                VStack(alignment: .leading) {
                                    Text(entry.grapheme)
                                    Text(detail(of: entry))
                                        .font(.footnote)
                                        .foregroundStyle(.secondary)
                                }
                                .frame(maxWidth: .infinity, alignment: .leading)
                                // Rows open an editor sheet rather than push,
                                // so they need an explicit tap affordance.
                                Image(systemName: "chevron.right")
                                    .font(.footnote.weight(.semibold))
                                    .foregroundStyle(.tertiary)
                                    .accessibilityHidden(true)
                            }
                            .contentShape(Rectangle())
                        }
                        .buttonStyle(.plain)
                        .accessibilityLabel("\(entry.grapheme), \(detail(of: entry))")
                        .accessibilityHint(Text("Edit"))
                        .swipeActions(edge: .trailing) {
                            Button(role: .destructive) {
                                entryPendingDeletion = entry
                            } label: {
                                Label("Delete", systemImage: "trash")
                            }
                            Button {
                                duplicate(entry)
                            } label: {
                                Label("Duplicate", systemImage: "doc.on.doc")
                            }
                        }
                        .contextMenu {
                            Button {
                                editorRoute = EditorRoute(entry: entry)
                            } label: {
                                Label("Edit", systemImage: "pencil")
                            }
                            Button {
                                duplicate(entry)
                            } label: {
                                Label("Duplicate", systemImage: "doc.on.doc")
                            }
                            Button(role: .destructive) {
                                entryPendingDeletion = entry
                            } label: {
                                Label("Delete", systemImage: "trash")
                            }
                        }
                    }
                    .onDelete { offsets in
                        delete(at: offsets)
                    }
                }
            }

            if let errorMessage {
                Section {
                    Text("Error: \(errorMessage)")
                        .foregroundStyle(.red)
                }
            }
        }
        .navigationTitle("Dictionaries")
        .inlineNavigationBarTitle()
        .toolbar {
            #if os(iOS)
            ToolbarItem(placement: .navigationBarLeading) {
                EditButton()
            }
            #endif
            ToolbarItem(placement: .primaryAction) {
                Button {
                    editorRoute = EditorRoute(entry: nil)
                } label: {
                    Label("Add entry", systemImage: "plus")
                }
            }
        }
        .sheet(item: $editorRoute) { route in
            NavigationStack {
                DictionaryEditView(
                    type: selectedType,
                    entry: route.entry,
                    onSave: { upsert($0) },
                    onDelete: { delete($0) }
                )
            }
            #if os(macOS)
            .frame(minWidth: 420, minHeight: 420)
            #endif
        }
        .alert(
            Text("Delete entry?"),
            isPresented: Binding(
                get: { entryPendingDeletion != nil },
                set: { if !$0 { entryPendingDeletion = nil } }
            )
        ) {
            Button("Delete", role: .destructive) {
                if let entry = entryPendingDeletion {
                    delete(entry)
                }
                entryPendingDeletion = nil
            }
            Button("Cancel", role: .cancel) {
                entryPendingDeletion = nil
            }
        } message: {
            Text("Are you sure you want to delete this dictionary entry?")
        }
        .task(id: selectedType) {
            reload()
        }
    }

    // MARK: Store operations

    private func reload() {
        do {
            entries = try model.dictionaries.load(selectedType)
            errorMessage = nil
        } catch {
            entries = []
            errorMessage = error.localizedDescription
        }
        isLoading = false
    }

    private func persist() {
        do {
            try model.dictionaries.save(entries, type: selectedType)
            errorMessage = nil
        } catch {
            errorMessage = error.localizedDescription
        }
    }

    private func upsert(_ entry: DictionaryEntry) {
        if let index = entries.firstIndex(where: { $0.id == entry.id }) {
            entries[index] = entry
        } else {
            entries.append(entry)
        }
        persist()
    }

    private func delete(_ entry: DictionaryEntry) {
        entries.removeAll { $0.id == entry.id }
        persist()
    }

    private func delete(at offsets: IndexSet) {
        // Route edit-mode deletion through the same "Delete entry?"
        // confirmation every other delete gesture uses.
        guard let index = offsets.first, entries.indices.contains(index) else { return }
        entryPendingDeletion = entries[index]
    }

    private func duplicate(_ entry: DictionaryEntry) {
        var copy = entry
        copy.id = UUID()
        // An accent entry is copied as it is: " (copy)" would make it a
        // different, unaccented word.
        if selectedType != .accents {
            copy.grapheme = String(localized: "\(entry.grapheme) (copy)")
        }
        entries.append(copy)
        persist()
    }

    /// The second line of a row: the replacement, or for the accent lexicon
    /// the entry's kind, language and comment.
    private func detail(of entry: DictionaryEntry) -> String {
        guard selectedType == .accents else { return entry.phoneme }
        let kind = entry.isVerb ? String(localized: "Verb") : String(localized: "Word")
        let language = accentLanguageName(entry.language)
        return entry.comment.isEmpty ? "\(kind), \(language)" : "\(kind), \(language), \(entry.comment)"
    }
}

/// The language of an accent lexicon entry as the user reads it.
func accentLanguageName(_ code: String) -> String {
    switch code {
    case "hr": return String(localized: "Croatian")
    case "sr": return String(localized: "Serbian")
    case "bs": return String(localized: "Bosnian")
    case "": return String(localized: "All languages")
    default: return code
    }
}

#Preview {
    NavigationStack {
        DictionaryListView()
    }
    .environmentObject(AppModel())
}

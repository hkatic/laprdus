// AboutView.swift - App information, legal links, and support.

import SwiftUI

struct AboutView: View {
    private var versionString: String {
        Bundle.main.object(forInfoDictionaryKey: "CFBundleShortVersionString") as? String ?? "1.0.0"
    }

    private var buildString: String {
        Bundle.main.object(forInfoDictionaryKey: "CFBundleVersion") as? String ?? "1"
    }

    var body: some View {
        List {
            Section("Application Information") {
                Text("Laprdus Retro TTS (Text-To-Speech)")
                Text("Version: \(versionString) (Build \(buildString))")
                Text("Copyright © 2026., Hrvoje Katić")
            }

            Section("Legal") {
                Link(destination: URL(string: "https://hrvojekatic.com/laprdus/privacy-statement.php")!) {
                    LinkRow(title: String(localized: "Read the Privacy Policy Online"))
                }
                Link(destination: URL(string: "https://www.gnu.org/licenses/gpl-3.0.en.html")!) {
                    LinkRow(title: String(localized: "Read the GPL 3.0 License Online"))
                }
            }

            Section("Support") {
                Link(destination: URL(string: "mailto:hrvojekatic@gmail.com")!) {
                    LinkRow(title: String(localized: "Contact author via E-Mail"))
                }
            }
        }
        .navigationTitle("About Laprdus")
        .inlineNavigationBarTitle()
    }
}

private struct LinkRow: View {
    let title: String

    var body: some View {
        HStack {
            Text(title)
            Spacer()
            Image(systemName: "arrow.up.right")
                .font(.footnote)
                .foregroundStyle(.secondary)
        }
        .contentShape(Rectangle())
        .accessibilityElement(children: .combine)
    }
}

#Preview {
    NavigationStack {
        AboutView()
    }
}

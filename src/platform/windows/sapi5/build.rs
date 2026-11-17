//! Compiles the version information of the DLL (`laprdus_sapi5.rc`).

fn main() {
    println!("cargo:rerun-if-changed=laprdus_sapi5.rc");
    #[cfg(windows)]
    embed_resource::compile_for_cdylib("laprdus_sapi5.rc", embed_resource::NONE)
        .manifest_optional()
        .expect("laprdus_sapi5.rc compiles");
}

//! Turns `resource.h` into Rust constants, so that the program and the
//! resource script share one list of ids, and compiles `laprdus_config.rc`.

use std::path::Path;
use std::{env, fs};

fn main() {
    for file in ["resource.h", "laprdus_config.rc", "laprdus_config.manifest"] {
        println!("cargo:rerun-if-changed={file}");
    }

    let header = fs::read_to_string("resource.h").expect("resource.h is next to build.rs");
    let mut ids = String::new();
    for line in header.lines() {
        let mut words = line.split_whitespace();
        if let (Some("#define"), Some(name), Some(value)) = (words.next(), words.next(), words.next())
            && let Ok(value) = value.parse::<u16>()
        {
            ids += &format!("pub const {name}: u16 = {value};\n");
        }
    }
    let out = env::var("OUT_DIR").expect("cargo sets OUT_DIR");
    fs::write(Path::new(&out).join("resource.rs"), ids).expect("OUT_DIR is writable");

    #[cfg(windows)]
    embed_resource::compile("laprdus_config.rc", embed_resource::NONE)
        .manifest_required()
        .expect("laprdus_config.rc compiles");
}

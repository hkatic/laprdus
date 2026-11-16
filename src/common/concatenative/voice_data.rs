//! The recordings of a concatenative voice: one sound per file, 16-bit PCM
//! mono at 22050 Hz, loaded from a packed `.bin` file or from a directory of
//! WAV files.

use crate::SAMPLE_RATE;
use std::fmt;
use std::path::Path;

/// The sounds a voice is recorded as: the letters of the alphabet and the
/// Croatian ones, the two digraphs, and the pause.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
#[repr(u8)]
pub enum Sound {
    A,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T,
    U,
    V,
    W,
    X,
    Y,
    Z,
    /// č.
    Ch,
    /// ć.
    Tj,
    /// đ and dž.
    Dj,
    /// š.
    Sh,
    /// ž.
    Zh,
    Lj,
    Nj,
    /// The pause recording (`-.wav`).
    Silence,
}

impl Sound {
    pub const ALL: [Sound; 34] = {
        use Sound::*;
        [
            A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z, Ch, Tj, Dj, Sh, Zh, Lj, Nj,
            Silence,
        ]
    };
    pub const COUNT: usize = Self::ALL.len();

    /// Name of the sound's WAV file.
    pub fn file_name(self) -> String {
        match self {
            Sound::Silence => "-.wav".into(),
            _ => format!("PHONEME_{}.wav", format!("{self:?}").to_uppercase()),
        }
    }

    /// The two digraphs are optional in a directory of recordings.
    fn optional(self) -> bool {
        matches!(self, Sound::Lj | Sound::Nj)
    }
}

/// Why a voice could not be loaded.
#[derive(Debug)]
pub enum VoiceDataError {
    Io(std::io::Error),
    /// Not a packed voice file of this version.
    Format,
    /// The file is encrypted and no key was given.
    KeyRequired,
    /// A directory without all the recordings.
    Incomplete,
}

impl fmt::Display for VoiceDataError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::Io(error) => write!(f, "cannot read the voice data: {error}"),
            Self::Format => f.write_str("not a Laprdus voice file"),
            Self::KeyRequired => f.write_str("the voice file is encrypted and needs a key"),
            Self::Incomplete => f.write_str("the directory lacks some of the recordings"),
        }
    }
}

impl std::error::Error for VoiceDataError {}

impl From<std::io::Error> for VoiceDataError {
    fn from(error: std::io::Error) -> Self {
        Self::Io(error)
    }
}

// The packed file: a 64-byte header, an index of 32-byte entries, the audio.
pub const FILE_MAGIC: u32 = 0x4C50_5244; // "LPRD"
pub const FILE_VERSION: u16 = 1;
pub const HEADER_SIZE: usize = 64;
pub const INDEX_ENTRY_SIZE: usize = 32;
pub const FLAG_ENCRYPTED: u16 = 0x0001;

fn u16_at(data: &[u8], at: usize) -> Option<u16> {
    Some(u16::from_le_bytes(data.get(at..at + 2)?.try_into().ok()?))
}

fn u32_at(data: &[u8], at: usize) -> Option<u32> {
    Some(u32::from_le_bytes(data.get(at..at + 4)?.try_into().ok()?))
}

fn samples_of(bytes: &[u8]) -> Vec<i16> {
    bytes.chunks_exact(2).map(|pair| i16::from_le_bytes([pair[0], pair[1]])).collect()
}

/// The recordings of one voice.
#[derive(Clone, Debug)]
pub struct VoiceData {
    sounds: [Vec<i16>; Sound::COUNT],
    sample_rate: u32,
}

impl VoiceData {
    fn empty() -> Self {
        Self { sounds: std::array::from_fn(|_| Vec::new()), sample_rate: SAMPLE_RATE }
    }

    /// Reads a packed voice file from memory; `key` decrypts it (empty for a
    /// plain file).
    pub fn from_packed(data: &[u8], key: &[u8]) -> Result<Self, VoiceDataError> {
        Self::parse(data, key).ok_or(VoiceDataError::Format)?
    }

    fn parse(data: &[u8], key: &[u8]) -> Option<Result<Self, VoiceDataError>> {
        if data.len() < HEADER_SIZE
            || u32_at(data, 0)? != FILE_MAGIC
            || u16_at(data, 4)? != FILE_VERSION
            || u32_at(data, 20)? as usize > data.len()
        {
            return None;
        }
        let encrypted = u16_at(data, 6)? & FLAG_ENCRYPTED != 0;
        if encrypted && key.is_empty() {
            return Some(Err(VoiceDataError::KeyRequired));
        }
        let count = u32_at(data, 8)? as usize;
        let index = data.get(u32_at(data, 12)? as usize..)?;
        let mut audio = data.get(u32_at(data, 16)? as usize..)?.to_vec();
        if encrypted {
            for (byte, key) in audio.iter_mut().zip(key.iter().cycle()) {
                *byte ^= key;
            }
        }

        let mut voice = Self { sample_rate: u32_at(data, 24)?, ..Self::empty() };
        for entry in index.chunks_exact(INDEX_ENTRY_SIZE).take(count) {
            let (id, offset, size) =
                (u32_at(entry, 0)? as usize, u32_at(entry, 8)? as usize, u32_at(entry, 16)? as usize);
            let bytes = offset.checked_add(size).and_then(|end| audio.get(offset..end));
            if let (Some(sound), Some(bytes)) = (voice.sounds.get_mut(id), bytes) {
                *sound = samples_of(bytes);
            }
        }
        Some(Ok(voice))
    }

    /// Reads a packed voice file.
    pub fn from_file(path: &Path, key: &[u8]) -> Result<Self, VoiceDataError> {
        Self::from_packed(&std::fs::read(path)?, key)
    }

    /// Reads a directory of `PHONEME_*.wav` files (development mode).
    pub fn from_directory(dir: &Path) -> Result<Self, VoiceDataError> {
        let mut voice = Self::empty();
        for sound in Sound::ALL {
            let Ok(file) = std::fs::read(dir.join(sound.file_name())) else { continue };
            if let Some((sample_rate, samples)) = read_wav(&file) {
                voice.sounds[sound as usize] = samples;
                if voice.sample_rate == SAMPLE_RATE {
                    voice.sample_rate = sample_rate;
                }
            }
        }
        let complete = Sound::ALL.iter().all(|&sound| sound.optional() || !voice.sound(sound).is_empty());
        if complete { Ok(voice) } else { Err(VoiceDataError::Incomplete) }
    }

    /// The recording of a sound, empty when the voice has none.
    pub fn sound(&self, sound: Sound) -> &[i16] {
        &self.sounds[sound as usize]
    }

    /// Sample rate of the recordings.
    pub fn sample_rate(&self) -> u32 {
        self.sample_rate
    }

    /// Bytes of audio held.
    pub fn memory_usage(&self) -> usize {
        self.sounds.iter().map(|sound| sound.len() * size_of::<i16>()).sum()
    }
}

/// Sample rate and samples of a 16-bit PCM WAV file.
fn read_wav(file: &[u8]) -> Option<(u32, Vec<i16>)> {
    if file.get(0..4)? != b"RIFF" || file.get(8..12)? != b"WAVE" || file.get(12..16)? != b"fmt " {
        return None;
    }
    let sample_rate = u32_at(file, 24)?;
    // The chunks after the format chunk, up to the audio.
    let mut at = 20 + u32_at(file, 16)?.max(16) as usize;
    loop {
        let size = u32_at(file, at + 4)? as usize;
        let body = at + 8;
        if file.get(at..at + 4)? == b"data" {
            return Some((sample_rate, samples_of(&file[body..file.len().min(body + size)])));
        }
        at = body + size;
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn file_names() {
        assert_eq!(Sound::A.file_name(), "PHONEME_A.wav");
        assert_eq!(Sound::Ch.file_name(), "PHONEME_CH.wav");
        assert_eq!(Sound::Silence.file_name(), "-.wav");
        assert_eq!(Sound::ALL[33] as usize, 33);
    }

    #[test]
    fn rejects_what_is_not_a_voice() {
        assert!(matches!(VoiceData::from_packed(b"RIFF", &[]), Err(VoiceDataError::Format)));
        assert!(matches!(VoiceData::from_packed(&[0; 64], &[]), Err(VoiceDataError::Format)));
    }
}

use std::{
    fs::{metadata, remove_file, rename, OpenOptions},
    io::{Read, Seek, SeekFrom::Start, Write},
    str::from_utf8,
    time::SystemTime,
};

use chrono::{TimeZone, Utc};

fn main() {
    let stat = metadata("test_file.txt").unwrap();
    println!("File size (in bytes): {}", stat.len());
    let timestamp = stat
        .modified()
        .unwrap()
        .duration_since(SystemTime::UNIX_EPOCH)
        .unwrap()
        .as_secs();
    let date = Utc
        .timestamp_opt(timestamp as i64, 0)
        .unwrap()
        .to_rfc2822()
        .replace(" +0000", "");
    println!("Last modified time: {}", date);

    let mut file = OpenOptions::new()
        .read(true)
        .write(true)
        .open("test_file.txt")
        .unwrap();
    let mut buf = [0u8; 5];
    let status = file.read(&mut buf).unwrap();
    println!("{} {}", status, from_utf8(&buf).unwrap());
    file.seek(Start(0)).unwrap();
    let mut buf2 = [0u8; 6];
    let status = file.read(&mut buf2).unwrap();
    println!(
        "{} {}{}",
        status,
        from_utf8(&buf).unwrap(),
        from_utf8(&buf2).unwrap()
    );
    file.write("word".as_bytes()).unwrap();
    file.set_len(30).unwrap();
    file.sync_all().unwrap();

    rename("test_file.txt", "renamed_file.txt").unwrap();
    remove_file("deleted_file.txt").unwrap();
}

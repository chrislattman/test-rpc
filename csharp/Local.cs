FileInfo f = new("test_file.txt");
Console.WriteLine("File size (in bytes): " + f.Length);
Console.WriteLine("Last modified time: " + f.LastWriteTimeUtc.ToString("R"));

FileStream file = new("test_file.txt", FileMode.OpenOrCreate, FileAccess.ReadWrite);
byte[] buf = new byte[200];
int status = file.Read(buf, 0, 5);
Console.WriteLine(status + " " + System.Text.Encoding.UTF8.GetString(buf));
file.Seek(0, SeekOrigin.Begin);
status = file.Read(buf, 5, 6);
Console.WriteLine(status + " " + System.Text.Encoding.UTF8.GetString(buf));
file.Write(System.Text.Encoding.UTF8.GetBytes("word"), 0, 4);
file.SetLength(30);
file.Flush();
file.Close();

f.MoveTo("renamed_file.txt");
File.Delete("deleted_file.txt");

var namelist = Directory.EnumerateFileSystemEntries(".");
foreach (var name in namelist)
{
    FileAttributes attributes = File.GetAttributes(name);
    if ((attributes & FileAttributes.Directory) == FileAttributes.Directory)
    {
        Console.WriteLine(name[2..] + "/");
    }
    if ((attributes & FileAttributes.Normal) == FileAttributes.Normal)
    {
        Console.WriteLine(name[2..]);
    }
}

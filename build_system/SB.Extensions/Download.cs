using Serilog;
using System.Collections.Generic;
using System.Diagnostics;
using System.Net;
using System.Security.Cryptography;
using System.Threading.Tasks;

namespace SB
{  
    public static class Download
    {
        static Download()
        {
            Directory.CreateDirectory(Engine.DownloadDirectory);
            AddSource("skr_github", "https://github.com/SakuraEngine/Sakura.Resources/releases/download/SDKs/");
        }

        public static void AddSource(string Name, string URL)
        {
            Sources.Add(Name, new DownloadSource
            {
                Name = Name,
                URL = URL,
                Destination = Engine.DownloadDirectory,
                ManifestPath = Path.Combine(Engine.DownloadDirectory, "manifests", $"{Name}.json")
            });
        }

        private static bool FetchOnceFlag = false;
        private static readonly object FetchLock = new object();
        public static void FetchManifests()
        {
            lock (FetchLock)
            {
                if (!FetchOnceFlag)
                {
                    FetchOnceFlag = true;

                    Directory.CreateDirectory(Path.Combine(Engine.DownloadDirectory, "manifests"));
                    foreach (var Source in Sources.Values)
                    {
                        var URL = Source.URL + "manifest.json";
                        try
                        {
                            Log.Information("fetching manifest ... from {URL} to {SourceManifestPath}", URL, Source.ManifestPath);
                            using (var Http = new HttpClient())
                            {
                                var Bytes = Http.GetByteArrayAsync(URL);
                                Bytes.Wait();
                                Source.ManifestString = Bytes.Result;
                                // write to disk, just for debugging
                                File.WriteAllBytes(Source.ManifestPath, Source.ManifestString);
                            }
                        }
                        catch (Exception e)
                        {
                            Log.Error(e, "Failed to fetch manifest from {URL}, message: {eMessage}", URL, e.Message);
                        }
                    }
                    LoadManifests();
                }
            }
        }

        private static void LoadManifests()
        {
            foreach (var Source in Sources.Values)
            {
                var JSONReader = new System.Text.Json.Utf8JsonReader(Source.ManifestString);
                JSONReader.Read(); // ROOT
                while (JSONReader.Read())
                {
                    if (JSONReader.TokenType == System.Text.Json.JsonTokenType.PropertyName)
                    {
                        var NAME = JSONReader.GetString()!;
                        JSONReader.Read();
                        var SHA = JSONReader.GetString()!;
                        // add result to source list
                        List<DownloadSource> SourcesList;
                        if (!FileSources.TryGetValue(NAME, out SourcesList!))
                        {
                            SourcesList = new List<DownloadSource>();
                            FileSources.Add(NAME, SourcesList);
                        }
                        SourcesList.Add(Source);
                        // add result to sha checker
                        HashSet<string> SHAs;
                        if (!FileSHAs.TryGetValue(NAME, out SHAs!))
                        {
                            SHAs = new HashSet<string>();
                            FileSHAs.Add(NAME, SHAs);
                        }
                        SHAs.Add(SHA);
                    }
                }
            }
            // check sha conflict
            FileSHAs.Where(static KVP => KVP.Value.Count > 1).ToList().ForEach(static KVP => { 
                throw new Exception($"{KVP.Key} SHA conflict detected!");
            });
            // print manifest info
            Log.Information("----------------manifest info----------------");
            foreach (var Source in Sources.Values)
            {
                Log.Information("{SourceName} ... {SourceURL}", Source.Name, Source.URL);
            }
            Log.Information("----------------manifest info----------------");
        }

        private static async Task DownloadFromSource(DownloadSource Source, string FileName)
        {
            var Destination = Path.Combine(Engine.DownloadDirectory, FileName);
            var URL = Source.URL + FileName;
            Log.Information("downloading ... from {URL} to {Destination}", URL, Destination);
            using (var Http = new HttpClient())
            {
                var Bytes = Http.GetByteArrayAsync(URL);
                await Bytes;
                await File.WriteAllBytesAsync(Destination, Bytes.Result);
            }
        }

        public static async Task<string> DownloadFile(string FileName, bool Force = false)
        {
            Download.FetchManifests();

            var FilePath = Path.Combine(Engine.DownloadDirectory, FileName);
            if (!Force && File.Exists(FilePath))
            {
                var ExistedSHA = Convert.ToHexString(SHA256.HashData(File.ReadAllBytes(Path.Combine(Engine.DownloadDirectory, FileName)))).ToUpperInvariant();
                var ManifestSHA = FileSHAs[FileName].First().ToUpperInvariant();
                if (ExistedSHA == ManifestSHA)
                {
                    Log.Information("downloading ... restore existed {FileName}", FileName);
                    return FilePath;
                }
            }
            await DownloadFromSource(Sources.Values.First(), FileName);
            return FilePath;
        }

        private static Dictionary<string, HashSet<string>> FileSHAs = new();
        private static Dictionary<string, List<DownloadSource>> FileSources = new();
        private static Dictionary<string, DownloadSource> Sources = new();
    }

    public class DownloadSource
    {
        public required string Name { get; init; }
        public required string URL { get; init; }
        public required string Destination { get; init; }
        public required string ManifestPath { get; init; }
        public Byte[]? ManifestString { get; set; }
    }
}

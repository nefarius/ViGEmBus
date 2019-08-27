using JsonConfig;
using Nuke.Common;
using Nuke.Common.BuildServers;
using Nuke.Common.Git;
using Nuke.Common.ProjectModel;
using Nuke.Common.Tools.GitVersion;
using Nuke.Common.Tools.MSBuild;
using System;
using System.IO;
using Vestris.ResourceLib;
using static Nuke.Common.IO.FileSystemTasks;
using static Nuke.Common.IO.PathConstruction;
using static Nuke.Common.Tools.MSBuild.MSBuildTasks;

class Build : NukeBuild
{
    [Parameter("Configuration to build - Default is 'Debug' (local) or 'Release' (server)")]
    private readonly string Configuration = IsLocalBuild ? "Debug_DLL" : "Release_DLL";

    [GitRepository] private readonly GitRepository GitRepository;
    [GitVersion] private readonly GitVersion GitVersion;

    [Solution("ViGEmClient.sln")] private readonly Solution Solution;
    private AbsolutePath ArtifactsDirectory => RootDirectory / "bin";

    private Target Clean => _ => _
        .Executes(() => { EnsureCleanDirectory(ArtifactsDirectory); });

    private Target Restore => _ => _
        .DependsOn(Clean)
        .Executes(() =>
        {
            MSBuild(s => s
                .SetTargetPath(Solution)
                .SetTargets("Restore"));
        });

    private Target Compile => _ => _
        .DependsOn(Restore)
        .Executes(() =>
        {
            MSBuild(s => s
                .SetTargetPath(Solution)
                .SetTargets("Rebuild")
                .SetConfiguration(Configuration)
                .SetMaxCpuCount(Environment.ProcessorCount)
                .SetNodeReuse(IsLocalBuild)
                .SetTargetPlatform(MSBuildTargetPlatform.x64));

            MSBuild(s => s
                .SetTargetPath(Solution)
                .SetTargets("Rebuild")
                .SetConfiguration(Configuration)
                .SetMaxCpuCount(Environment.ProcessorCount)
                .SetNodeReuse(IsLocalBuild)
                .SetTargetPlatform(MSBuildTargetPlatform.x86));

            var version =
                new Version(IsLocalBuild ? GitVersion.GetNormalizedFileVersion() : AppVeyor.Instance.BuildVersion);

            Console.WriteLine($"Stamping version {version}");

            StampVersion(
                Path.Combine(
                    RootDirectory,
                    $@"bin\{Configuration.Substring(0, Configuration.Length - 4)}\x64\ViGEmClient.dll"),
                version);

            StampVersion(
                Path.Combine(
                    RootDirectory,
                    $@"bin\{Configuration.Substring(0, Configuration.Length - 4)}\x86\ViGEmClient.dll"),
                version);
        });

    private Target Pack => _ => _
        .DependsOn(Compile)
        .Executes(() =>
        {
            MSBuild(s => s
                .SetTargetPath(Solution)
                .SetTargets("Restore", "Pack")
                .SetPackageOutputPath(ArtifactsDirectory)
                .SetConfiguration(Configuration)
                .EnableIncludeSymbols());
        });

    public static int Main()
    {
        return Execute<Build>(x => x.Compile);
    }

    private static void StampVersion(string path, Version version)
    {
        var versionResource = new VersionResource
        {
            FileVersion = version.ToString(),
            ProductVersion = version.ToString()
        };

        var stringFileInfo = new StringFileInfo();
        versionResource[stringFileInfo.Key] = stringFileInfo;
        var stringFileInfoStrings = new StringTable
        {
            LanguageID = 1033,
            CodePage = 1200
        };
        stringFileInfo.Strings.Add(stringFileInfoStrings.Key, stringFileInfoStrings);
        stringFileInfoStrings["CompanyName"] = Config.Global.Version.CompanyName;
        stringFileInfoStrings["FileDescription"] = Config.Global.Version.FileDescription;
        stringFileInfoStrings["FileVersion"] = version.ToString();
        stringFileInfoStrings["InternalName"] = Config.Global.Version.InternalName;
        stringFileInfoStrings["LegalCopyright"] = Config.Global.Version.LegalCopyright;
        stringFileInfoStrings["OriginalFilename"] = Config.Global.Version.OriginalFilename;
        stringFileInfoStrings["ProductName"] = Config.Global.Version.ProductName;
        stringFileInfoStrings["ProductVersion"] = version.ToString();

        var varFileInfo = new VarFileInfo();
        versionResource[varFileInfo.Key] = varFileInfo;
        var varFileInfoTranslation = new VarTable("Translation");
        varFileInfo.Vars.Add(varFileInfoTranslation.Key, varFileInfoTranslation);
        varFileInfoTranslation[ResourceUtil.USENGLISHLANGID] = 1300;

        versionResource.SaveTo(path);
    }
}

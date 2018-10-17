using System;
using System.IO;
using Nuke.Common;
using Nuke.Common.BuildServers;
using Nuke.Common.Git;
using Nuke.Common.ProjectModel;
using Nuke.Common.Tools.GitVersion;
using Nuke.Common.Tools.MSBuild;
using Vestris.ResourceLib;
using static Nuke.Common.EnvironmentInfo;
using static Nuke.Common.IO.FileSystemTasks;
using static Nuke.Common.IO.PathConstruction;
using static Nuke.Common.Tools.MSBuild.MSBuildTasks;

internal class Build : NukeBuild
{
    [GitRepository] private readonly GitRepository GitRepository;
    [GitVersion] private readonly GitVersion GitVersion;

    [Solution("ViGEmBus.sln")] private readonly Solution Solution;

    private AbsolutePath ArtifactsDirectory => RootDirectory / "artifacts";

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

            #region Ugly hack, fix me!

            EnsureExistingDirectory(Path.Combine(ArtifactsDirectory, @"x64"));
            EnsureExistingDirectory(Path.Combine(ArtifactsDirectory, @"x86"));

            File.Copy(
                Path.Combine(WorkingDirectory, @"bin\x64\ViGEmBus.inf"),
                Path.Combine(ArtifactsDirectory, @"ViGEmBus.inf")
            );

            File.Copy(
                Path.Combine(WorkingDirectory, @"bin\x64\ViGEmBus.pdb"),
                Path.Combine(ArtifactsDirectory, @"x64\ViGEmBus.pdb")
            );
            File.Copy(
                Path.Combine(WorkingDirectory, @"bin\x64\ViGEmBus\ViGEmBus.sys"),
                Path.Combine(ArtifactsDirectory, @"x64\ViGEmBus.sys")
            );
            File.Copy(
                Path.Combine(WorkingDirectory, @"bin\x64\ViGEmBus\WdfCoinstaller01009.dll"),
                Path.Combine(ArtifactsDirectory, @"x64\WdfCoinstaller01009.dll")
            );

            File.Copy(
                Path.Combine(WorkingDirectory, @"bin\x86\ViGEmBus.pdb"),
                Path.Combine(ArtifactsDirectory, @"x86\ViGEmBus.pdb")
            );
            File.Copy(
                Path.Combine(WorkingDirectory, @"bin\x86\ViGEmBus\ViGEmBus.sys"),
                Path.Combine(ArtifactsDirectory, @"x86\ViGEmBus.sys")
            );
            File.Copy(
                Path.Combine(WorkingDirectory, @"bin\x86\ViGEmBus\WdfCoinstaller01009.dll"),
                Path.Combine(ArtifactsDirectory, @"x86\WdfCoinstaller01009.dll")
            );

            #endregion
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
}
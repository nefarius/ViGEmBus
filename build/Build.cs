using System;
using System.IO;
using System.Linq;
using Nuke.Common;
using Nuke.Common.Git;
using Nuke.Common.ProjectModel;
using Nuke.Common.Tools.MSBuild;
using static Nuke.Common.EnvironmentInfo;
using static Nuke.Common.IO.FileSystemTasks;
using static Nuke.Common.IO.PathConstruction;
using static Nuke.Common.Tools.MSBuild.MSBuildTasks;

class Build : NukeBuild
{
    public static int Main () => Execute<Build>(x => x.Compile);

    [Solution] readonly Solution Solution;
    [GitRepository] readonly GitRepository GitRepository;

    AbsolutePath ArtifactsDirectory => RootDirectory / "artifacts";

    Target Clean => _ => _
        .Executes(() =>
        {
            EnsureCleanDirectory(ArtifactsDirectory);
        });

    Target Restore => _ => _
        .DependsOn(Clean)
        .Executes(() =>
        {
            MSBuild(s => s
                .SetTargetPath(SolutionFile)
                .SetTargets("Restore"));
        });

    Target Compile => _ => _
        .DependsOn(Restore)
        .Executes(() =>
        {
            MSBuild(s => s
                .SetTargetPath(SolutionFile)
                .SetTargets("Rebuild")
                .SetConfiguration(Configuration)
                .SetMaxCpuCount(Environment.ProcessorCount)
                .SetNodeReuse(IsLocalBuild)
                .SetTargetPlatform(MSBuildTargetPlatform.x64));

            MSBuild(s => s
                .SetTargetPath(SolutionFile)
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
                .SetTargetPath(SolutionFile)
                .SetTargets("Restore", "Pack")
                .SetPackageOutputPath(ArtifactsDirectory)
                .SetConfiguration(Configuration)
                .EnableIncludeSymbols());
        });
}

using System;
using System.IO;
using System.Linq;
using Nuke.Common;
using Nuke.Common.Git;
using Nuke.Common.Tools.GitVersion;
using Nuke.Common.Tools.MSBuild;
using Nuke.Core;
using static Nuke.Common.Tools.MSBuild.MSBuildTasks;
using static Nuke.Core.IO.FileSystemTasks;
using static Nuke.Core.IO.PathConstruction;
using static Nuke.Core.EnvironmentInfo;

class Build : NukeBuild
{
    // Console application entry. Also defines the default target.
    public static int Main () => Execute<Build>(x => x.Compile);

    // Auto-injection fields:

    // [GitVersion] readonly GitVersion GitVersion;
    // Semantic versioning. Must have 'GitVersion.CommandLine' referenced.

    // [GitRepository] readonly GitRepository GitRepository;
    // Parses origin, branch name and head from git config.

    // [Parameter] readonly string MyGetApiKey;
    // Returns command-line arguments and environment variables.

    Target Clean => _ => _
            .OnlyWhen(() => false) // Disabled for safety.
            .Executes(() =>
            {
                DeleteDirectories(GlobDirectories(SourceDirectory, "**/bin", "**/obj"));
                EnsureCleanDirectory(OutputDirectory);
            });

    Target Restore => _ => _
            .DependsOn(Clean)
            .Executes(() =>
            {
                MSBuild(s => DefaultMSBuildRestore.SetTargetPlatform(MSBuildTargetPlatform.x64));
                MSBuild(s => DefaultMSBuildRestore.SetTargetPlatform(MSBuildTargetPlatform.x86));
            });

    Target Compile => _ => _
            .DependsOn(Restore)
            .Executes(() =>
            {
                MSBuild(s => DefaultMSBuildCompile.SetTargetPlatform(MSBuildTargetPlatform.x64));
                MSBuild(s => DefaultMSBuildCompile.SetTargetPlatform(MSBuildTargetPlatform.x86));

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
}

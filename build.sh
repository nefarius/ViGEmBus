#!/usr/bin/env bash

NOINIT=0
BUILD_ARGUMENTS=()
for i in "$@"; do
    case $(echo $1 | awk '{print tolower($0)}') in
        -noinit) NOINIT=1;;
        *) BUILD_ARGUMENTS+=("$1") ;;
    esac
    shift
done

set -eo pipefail
SCRIPT_DIR=$(cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd)

###########################################################################
# CONFIGURATION
###########################################################################

NUGET_VERSION="latest"
SOLUTION_DIRECTORY="$SCRIPT_DIR/../ViGEm"
BUILD_PROJECT_FILE="$SCRIPT_DIR/./build/.build.csproj"
BUILD_EXE_FILE="$SCRIPT_DIR/./build/bin/Debug/.build.exe"

TEMP_DIRECTORY="$SCRIPT_DIR/.tmp"

NUGET_URL="https://dist.nuget.org/win-x86-commandline/$NUGET_VERSION/nuget.exe"
NUGET_FILE="$TEMP_DIRECTORY/nuget.exe"
export NUGET_EXE="$NUGET_FILE"

###########################################################################
# PREPARE BUILD
###########################################################################

if ! ((NOINIT)); then
  mkdir -p "$TEMP_DIRECTORY"

  if [ ! -f "$NUGET_FILE" ]; then curl -Lsfo "$NUGET_FILE" $NUGET_URL;
  elif [ $NUGET_VERSION == "latest" ]; then mono "$NUGET_FILE" update -Self; fi

  mono "$NUGET_FILE" restore "$BUILD_PROJECT_FILE" -SolutionDirectory $SOLUTION_DIRECTORY
fi

msbuild "$BUILD_PROJECT_FILE"

###########################################################################
# EXECUTE BUILD
###########################################################################

mono "$BUILD_EXE_FILE" ${BUILD_ARGUMENTS[@]}

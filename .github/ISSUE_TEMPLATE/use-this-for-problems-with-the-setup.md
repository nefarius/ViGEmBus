---
name: Setup issues
about: Use this if you have problems with the setup

---

## README first

Before you report issues with the setup, make sure to check the following things **first**:

- Are you on anything older than Windows **10**? Windows XP/Vista/7/8/8.1 **are not supported** so don't report this as an issue, it is intentional.
- [Have you chosen the right setup for your CPU architecture?](https://vigem.org/research/How-to-check-architecture/)
  - You **can not mix these**, make sure to download and run the correct setup.
  - **ARM/ARM64** is **not supported**. See above link to check for yourself.

## I verified all that and am still stuck

Please provide the following **log files** and attach them to the issue. Failing to do so will result in the issue being closed without any further comment.

### Setup log

Run the setup from the command line (either old-school `cmd` or PowerShell) with the following additional arguments: `/L*V .\install.log`

This will generate the log file `install.log` in the same directory the setup resides in. Attach it once the setup is "done" failing.

Last but not least look for `C:\Windows\INF\setupapi.dev.log` and attach it as well.

Optionally compress both files down if it gives you troubles uploading.

Your compliance is apprechiated! 😘

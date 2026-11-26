# Security

## Scope

LogoFAILDemo is an academic and experimental UEFI firmware security project inspired by the LogoFAIL vulnerability class.

The current implementation provides a non-destructive arithmetic model of a BMP allocation-size inconsistency and an explicit demonstration marker.

It does not implement the original LogoFAIL memory corruption, actual control-flow hijacking, or native payload execution.

## Experimental Environment

The project is intended for controlled experimentation, primarily using EDK II and OVMF.

Testing on physical devices or third-party systems should only be performed with explicit authorization.

## Security Limitations

This project processes BMP files obtained from EFI System Partitions (ESP).

Although the demonstration does not intentionally perform memory corruption or modify firmware security variables, its parsing and image-decoding dependencies process external input and may have their own security limitations.

The project should not be considered production-ready firmware software or a security validation tool.

## Vulnerability Reporting

If you identify a potential security vulnerability in this project, please avoid publishing operational exploit details in public issues.

Instead, contact the maintainers privately, providing:

* A description of the affected component.
* The relevant EDK II and OVMF versions.
* Steps to reproduce the issue in an isolated environment.
* Relevant logs or diagnostic information.

Reports will be reviewed on a best-effort basis. As this is an academic project, no formal response time or vulnerability disclosure program is guaranteed.

## Responsible Use

Use this project only for educational purposes and authorized security research.

The maintainers do not endorse unauthorized firmware modification, access to third-party systems, or deployment of experimental code against systems without permission.

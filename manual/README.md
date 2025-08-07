# Contributed by ArgV-C-Transformer

## Version Pre - V0.0.0

ArgV-C version used - pre V0.0.0 experimental

## Description

Benchmarks created automagically from FLOSS projects on GitHub including:

- DrKLO/Telegram
- antirez/redis
- plexinc/plex-home-theater-public

Along with PACLab created benchmarks:

- triangle
- triangle1
- triangle2

With Modifications Including:

- adding necessary SV-Comp VERIFIER's
- ensure a main function exists
- ensure all functions are referenced and used at somepoint and can be
reached from the main function
- file names changed to unique names
- licenses included at beginning of all files

Filtered Based On:

- only using c standard libraries
- having minimum of 5 lines of code

Final Notes:

- Files are not preprocessed due to change in rule discussed in Community
meeting in April 2025
- YML files are provided with code
- Primary Set Targeted - ControlFlow.set
- Primary Categories Targeted:
  - ReachSafety-ControlFlow
- No witnesses were created or provided

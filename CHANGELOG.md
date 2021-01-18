```
                 _-====-__-=-__-===-__-=======-__
               _(                               _)
            OO(         PFAEDLE                )
         . o  '===-______-===-____-==-__-====='
      .o
     . ______          _______________
   _()_||__|| __o^o___ | [] [] [] [] |
  (           |      | |             |o
 /-OO----OO""="OO--OO"="OO---------OO"
############################################################
```

# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]
### Added
- changelog support
- use new dependency management system named [pkg](https://github.com/motis-project/pkg)
- new gtfs library underneath for handling gtfs feeds (much simpler and lighter)
- use of [pugixml](https://pugixml.org) instead of pfxml for parsing xml osm data
- enhanced logging support (using [spdlog](https://github.com/gabime/spdlog) for that)
- **WIP** clangformat and clang-tidy support 

### Changed
- releases for **2.*** versions will be done from branch **v2**
- use of new c++ standards as much as possible
- reorganized and changed class namings to make things much clearer (for me atleast)

### Removed
- usage of pfxml library for parsing osm data
- usage of cppgtfs for handling gtfs

## [1.0.0] - 2017-06-20 
- Initial baseline for this repo

## [0.1.6] - 04-02-2019
## [0.1.5] - 16-01-2019
## [0.1.4] - 13-01-2019
## [0.1.3] - 12-01-2019
## [0.1.2] - 23-07-2018
## [0.1.1] - 22-07-2018
## [0.1] - 12-07-2018

[Unreleased]: https://github.com/vesavlad/pfaedle/compare/v1.0.0...HEAD
[1.0.0]: https://github.com/vesavlad/pfaedle/compare/v0.1.6...v1.0.0
[0.1.6]: https://github.com/vesavlad/pfaedle/compare/v0.1.5...v0.1.6
[0.1.5]: https://github.com/vesavlad/pfaedle/compare/v0.1.4...v0.1.5
[0.1.4]: https://github.com/vesavlad/pfaedle/compare/v0.1.3...v0.1.4
[0.1.3]: https://github.com/vesavlad/pfaedle/compare/v0.1.2...v0.1.3
[0.1.2]: https://github.com/vesavlad/pfaedle/compare/v0.1.1...v0.1.2
[0.1.1]: https://github.com/vesavlad/pfaedle/compare/v0.1...v0.1.1
[0.1]: https://github.com/vesavlad/pfaedle/compare/efcd3e18926e97a637a7c72e21d08f079a05c4cc...v0.1


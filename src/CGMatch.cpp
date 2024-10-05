#include <glob.h>
#include <string>
#include <vector>
#include <unistd.h>
#include <iostream>
#include <sys/stat.h>

int
main(int argc, char **argv)
{
  enum class SummaryType {
    NONE,
    COUNT,
    NEWEST,
    OLDEST,
    LARGEST,
    SMALLEST,
  };

  using Strs = std::vector<std::string>;

  Strs        patterns;
  SummaryType summaryType = SummaryType::NONE;
  bool        mark        = false;
  bool        quiet       = false;
  std::string nostr;
  std::string includeTypes;
  std::string excludeTypes;
  Strs        execStrs;
  bool        ignoreArgs = false;

  for (int i = 1; i < argc; ++i) {
    if (! ignoreArgs && argv[i][0] == '-') {
      std::string arg = &argv[i][1];

      if      (arg == "-")
        ignoreArgs = true;
      else if (arg == "c" || arg == "-count")
        summaryType = SummaryType::COUNT;
      else if (arg == "-newest")
        summaryType = SummaryType::NEWEST;
      else if (arg == "-oldest")
        summaryType = SummaryType::OLDEST;
      else if (arg == "-largest")
        summaryType = SummaryType::LARGEST;
      else if (arg == "-smallest")
        summaryType = SummaryType::SMALLEST;
      else if (arg == "i" || arg == "-include") {
        ++i;

        if (i < argc)
          includeTypes = argv[i];
      }
      else if (arg == "x" || arg == "-exclude") {
        ++i;

        if (i < argc)
          excludeTypes = argv[i];
      }
      else if (arg == "m")
        mark = true;
      else if (arg == "q")
        quiet = true;
      else if (arg == "nostr") {
        ++i;

        if (i < argc)
          nostr = argv[i];
      }
      else if (arg == "e" || arg == "-exec") {
        ++i;

        if (i >= argc) {
          std::cerr << "Missing value for '" << argv[i] << "'\n";
          return -1;
        }

        while (i < argc)
          execStrs.push_back(argv[i++]);
      }
      else if (argv[i][1] == 'h') {
        auto &os = std::cerr;

        os << "CGMatch -c|--count --newest|--oldest|--largest|--smallest "
                       "-i|--include -x|--exclude -m -q -e|--exec <command> <pattern>\n";
        os << " -c|--count          : count matching files\n";
        os << " --newest            : newest matching file\n";
        os << " --oldest            : oldest matching file\n";
        os << " --largest           : largest matching file\n";
        os << " --smallest          : smallest matching file\n";
        os << " -i|--include        : include types\n";
        os << " -x|--exclude        : exclude types\n";
        os << " -m                  : add directory marker (/ at end)\n";
        os << " -q                  : suppress messages\n";
        os << " -nostr              : string to return if no match\n";
        os << " -e|--exec <command> : execute command on each matching file\n";
        return 0;
      }
      else
        std::cerr << "Invalid option '" << argv[i] << "'\n";
    }
    else {
      patterns.push_back(argv[i]);
    }
  }

  //---

  bool reg_type  = true; bool reg_specified  = false;
  bool dir_type  = true; bool dir_specified  = false;
  bool link_type = true; bool link_specified = false;

  if (! includeTypes.empty()) {
    reg_type  = false;
    dir_type  = false;
    link_type = false;

    for (int i = 0; i < int(includeTypes.size()); ++i) {
      if      (includeTypes[i] == 'r') {
        reg_type      = true;
        reg_specified = true;
      }
      else if (includeTypes[i] == 'd') {
        dir_type      = true;
        dir_specified = true;
      }
      else if (includeTypes[i] == 'l') {
        link_type      = true;
        link_specified = true;
      }
      else
        std::cerr << "Invalid include type '" << includeTypes[i] << "'\n";
    }
  }

  if (! excludeTypes.empty()) {
    for (int i = 0; i < int(excludeTypes.size()); ++i) {
      if      (excludeTypes[i] == 'r') {
        reg_type      = false;
        reg_specified = true;
      }
      else if (excludeTypes[i] == 'd') {
        dir_type      = false;
        dir_specified = true;
      }
      else if (excludeTypes[i] == 'l') {
        link_type      = false;
        link_specified = true;
      }
      else
        std::cerr << "Invalid exclude type '" << excludeTypes[i] << "'\n";
    }
  }

  //---

  // get matching files

  glob_t globbuf;

  int flags = 0;

#ifdef GLOB_BRACE
  flags |= GLOB_BRACE;
#endif

#ifdef GLOB_TILDE_CHECK
  flags |= GLOB_TILDE_CHECK;
#endif

  if (mark)
    flags |= GLOB_MARK;

  if (! execStrs.empty()) {
    globbuf.gl_offs = execStrs.size();

    flags |= GLOB_DOOFFS;
  }

  if (patterns.empty())
    patterns.push_back("*");

  int nomatch = 0;

  for (const auto &pattern : patterns) {
    int rc = glob(pattern.c_str(), flags, nullptr, &globbuf);

    if (rc == GLOB_NOMATCH)
      ++nomatch;

    flags |= GLOB_APPEND;
  }

  //---

  // if no files match then output message
  if (nomatch == int(patterns.size())) {
    std::string patternsStr;

    for (const auto &pattern : patterns) {
      if (! patternsStr.empty())
        patternsStr += ", ";

      patternsStr += "'" + pattern + "'";
    }

    if (! quiet)
      std::cerr << "No match for " << patternsStr << "\n";

    if      (summaryType == SummaryType::COUNT)
      std::cout << "0\n";
    else if (nostr != "")
      std::cout << nostr << "\n";

    return -1;
  }

  //---

  // filter files
  std::vector<char *> filenames;

  if (reg_specified || dir_specified || link_specified) {
    for (size_t i = 0; i < globbuf.gl_pathc; ++i) {
      if (link_specified) {
        struct stat stat1;

        lstat(globbuf.gl_pathv[i], &stat1);

        if (S_ISLNK(stat1.st_mode)) {
          if (link_type)
            filenames.push_back(globbuf.gl_pathv[i]);

          continue;
        }
      }

      struct stat stat2;

      stat (globbuf.gl_pathv[i], &stat2);

      if (dir_specified) {
        if (S_ISDIR(stat2.st_mode)) {
          if (dir_type)
            filenames.push_back(globbuf.gl_pathv[i]);

          continue;
        }
      }

      if (S_ISREG(stat2.st_mode)) {
        if (reg_type)
          filenames.push_back(globbuf.gl_pathv[i]);

        continue;
      }
    }
  }
  else {
    for (size_t i = 0; i < globbuf.gl_pathc; ++i)
      filenames.push_back(globbuf.gl_pathv[i]);
  }

  //---

  int rc = 0;

  // execute command on each file
  if      (! execStrs.empty()) {
    std::vector<char *> args = filenames;

    for (const auto &execStr : execStrs)
      args.push_back(const_cast<char *>(execStr.c_str()));

    // <command> <files> <extra_args>
    rc = execvp(execStrs[0].c_str(), &args[0]);
  }
  // display count
  else if (summaryType == SummaryType::COUNT) {
    std::cout << filenames.size() << "\n";

    rc = 0; // count ?
  }
  else if (summaryType == SummaryType::NEWEST || summaryType == SummaryType::OLDEST) {
    if (! filenames.empty()) {
      std::string bestFile;
      time_t      time { 0 };
      int         i { 0 };

      for (const auto &file : filenames) {
        struct stat stat1;

        lstat(file, &stat1);

        auto time1 = stat1.st_mtime;

        bool better = false;

        if (summaryType == SummaryType::NEWEST)
          better = (i == 0 || time1 > time);
        else
          better = (i == 0 || time1 < time);

        if (better) {
          bestFile = file;
          time     = time1;
        }

        ++i;
      }

      std::cout << bestFile << "\n";
    }
  }
  else if (summaryType == SummaryType::LARGEST || summaryType == SummaryType::SMALLEST) {
    if (! filenames.empty()) {
      std::string bestFile;
      long        size { 0 };
      int         i { 0 };

      for (const auto &file : filenames) {
        struct stat stat1;

        lstat(file, &stat1);

        auto size1 = stat1.st_size;

        bool better = false;

        if (summaryType == SummaryType::LARGEST)
          better = (i == 0 || size1 > size);
        else
          better = (i == 0 || size1 < size);

        if (better) {
          bestFile = file;
          size     = size1;
        }

        ++i;
      }

      std::cout << bestFile << "\n";
    }
  }
  // display files
  else {
    if (! filenames.empty()) {
      int i = 0;

      for (const auto &file : filenames) {
        if (i > 0) std::cout << " ";

        std::cout << file;

        ++i;
      }

      std::cout << "\n";
    }

    rc = int(filenames.size());
  }

  globfree(&globbuf);

  return rc;
}

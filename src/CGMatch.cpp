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
  Strs        execStrs;
  bool        ignore_args = false;

  for (int i = 1; i < argc; ++i) {
    if (! ignore_args && argv[i][0] == '-') {
      std::string arg = &argv[i][1];

      if      (arg == "-")
        ignore_args = true;
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
      else if (arg == "m")
        mark = true;
      else if (arg == "q")
        quiet = true;
      else if (arg == "nostr") {
        ++i;

        if (i < argc)
          nostr = argv[i];
      }
      else if (arg == "e") {
        ++i;

        if (i >= argc) {
          std::cerr << "Missing value for '" << argv[i] << "'\n";
          return -1;
        }

        while (i < argc)
          execStrs.push_back(argv[i++]);
      }
      else if (argv[i][1] == 'h') {
        std::cerr << "CGMatch [-c|--count|--newest|--oldest|--largest|--smallest|"
                     "-m|-q|-e <command>] <pattern>\n";
        std::cerr << " -c|--count   : count matching files\n";
        std::cerr << " -newest      : newest matching file\n";
        std::cerr << " -oldest      : oldest matching file\n";
        std::cerr << " -largest     : largest matching file\n";
        std::cerr << " -smallest    : smallest matching file\n";
        std::cerr << " -m           : add directory marker (/ at end)\n";
        std::cerr << " -q           : suppress messages\n";
        std::cerr << " -nostr       : string to return if no match\n";
        std::cerr << " -e <command> : execute command on each matching file\n";
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

    if (summaryType == SummaryType::COUNT)
      std::cout << "0\n";
    else if (nostr != "")
      std::cout << nostr << "\n";

    return -1;
  }

  //---

  int rc = 0;

  // execute command on each file
  if      (! execStrs.empty()) {
    int i = 0;

    for (const auto &execStr : execStrs)
      globbuf.gl_pathv[i++] = (char *) execStr.c_str();

    rc = execvp(execStrs[0].c_str(), &globbuf.gl_pathv[0]);
  }
  // display count
  else if (summaryType == SummaryType::COUNT) {
    std::cout << globbuf.gl_pathc << "\n";
  }
  else if (summaryType == SummaryType::NEWEST ||
           summaryType == SummaryType::OLDEST) {
    if (globbuf.gl_pathc) {
      std::string bestFile;
      time_t      time { 0 };

      for (size_t i = 0; i < globbuf.gl_pathc; ++i) {
        struct stat stat1;

        lstat(globbuf.gl_pathv[i], &stat1);

        auto time1 = stat1.st_mtime;

        bool better = false;

        if (summaryType == SummaryType::NEWEST)
          better = (i == 0 || time1 > time);
        else
          better = (i == 0 || time1 < time);

        if (better) {
          bestFile = globbuf.gl_pathv[i];
          time     = time1;
        }
      }

      std::cout << bestFile << "\n";
    }
  }
  else if (summaryType == SummaryType::LARGEST ||
           summaryType == SummaryType::SMALLEST) {
    if (globbuf.gl_pathc) {
      std::string bestFile;
      long        size { 0 };

      for (size_t i = 0; i < globbuf.gl_pathc; ++i) {
        struct stat stat1;

        lstat(globbuf.gl_pathv[i], &stat1);

        auto size1 = stat1.st_size;

        bool better = false;

        if (summaryType == SummaryType::LARGEST)
          better = (i == 0 || size1 > size);
        else
          better = (i == 0 || size1 < size);

        if (better) {
          bestFile = globbuf.gl_pathv[i];
          size     = size1;
        }
      }

      std::cout << bestFile << "\n";
    }
  }
  // display files
  else {
    if (globbuf.gl_pathc) {
      for (size_t i = 0; i < globbuf.gl_pathc; ++i) {
        if (i > 0) std::cout << " ";

        std::cout << globbuf.gl_pathv[i];
      }

      std::cout << "\n";
    }

    rc = globbuf.gl_pathc;
  }

  globfree(&globbuf);

  return rc;
}

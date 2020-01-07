#include <glob.h>
#include <string>
#include <vector>
#include <unistd.h>
#include <iostream>

int
main(int argc, char **argv)
{
  using Strs = std::vector<std::string>;

  Strs patterns;
  bool count       = false;
  bool mark        = false;
  bool quiet       = false;
  Strs execStrs;
  bool ignore_args = false;

  for (int i = 1; i < argc; ++i) {
    if (! ignore_args && argv[i][0] == '-') {
      if      (argv[i][1] == '-')
        ignore_args = true;
      else if (argv[i][1] == 'c')
        count = true;
      else if (argv[i][1] == 'm')
        mark = true;
      else if (argv[i][1] == 'q')
        quiet = true;
      else if (argv[i][1] == 'e') {
        ++i;

        if (i >= argc) {
          std::cerr << "Missing value for '" << argv[i] << "'\n";
          return -1;
        }

        while (i < argc)
          execStrs.push_back(argv[i++]);
      }
      else if (argv[i][1] == 'h') {
        std::cerr << "CGMatch [-c|-m|-q|-e <command>] <pattern>\n";
        return 0;
      }
      else
        std::cerr << "Invalid option '" << argv[i] << "'\n";
    }
    else {
      patterns.push_back(argv[i]);
    }
  }

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

  if (nomatch == int(patterns.size())) {
    std::string patternsStr;

    for (const auto &pattern : patterns) {
      if (! patternsStr.empty())
        patternsStr += ", ";

      patternsStr += "'" + pattern + "'";
    }

    if (! quiet)
      std::cerr << "No match for " << patternsStr << "\n";

    if (count)
      std::cout << "0\n";

    return -1;
  }

  int rc = 0;

  if      (! execStrs.empty()) {
    int i = 0;

    for (const auto &execStr : execStrs)
      globbuf.gl_pathv[i++] = (char *) execStr.c_str();

    rc = execvp(execStrs[0].c_str(), &globbuf.gl_pathv[0]);
  }
  else if (count) {
    std::cout << globbuf.gl_pathc << "\n";
  }
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

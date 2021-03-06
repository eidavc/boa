#ifndef __BOA_HELPERS_H
#define __BOA_HELPERS_H

#include <string>
using std::string;

namespace boa {

/**
 * Helper functions.
 *
 * This namespace should only have static methods.
 */
namespace Helpers {
  /**
   * Replaces each occurence of the character c with the string replaceWith.
   * Note - this mutates the string s.
   */
  void ReplaceInString(string &s, char c, const string& replaceWith);

  /**
   * Returns a string representation of the input.
   */
  string DoubleToString(double d);
}  // namespace Helpers

}  // namespace boa

#endif  // __BOA_HELPERS_H


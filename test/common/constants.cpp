#include "constants.h"

#include <fmt/format.h>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <chrono>

namespace tgnews {

using namespace boost::posix_time;

std::string MakeXmlDocument() {
  ptime t = second_clock::universal_time();
  return fmt::format(
      "<!DOCTYPE html>\n"
      "<html>\n"
      "  <head>\n"
      "    <meta charset=\"utf-8\"/>\n"
      "    <meta property=\"og:url\" content=\"https://sputniknews.com/science/202004301079143165-scientists-discover-potentially-oldest-mammal-dubbed-crazy-beast-in-madagascar/\"/>\n"
      "    <meta property=\"og:site_name\" content=\"Sputniknews\"/>\n"
      "    <meta property=\"article:published_time\" content=\"{}+00:00\"/>\n"
      "    <meta property=\"og:title\" content=\"Scientists Discover Potentially Oldest Mammal Dubbed 'Crazy Beast' in Madagascar\"/>\n"
      "    <meta property=\"og:description\" content=\"Researchers have discovered a nearly complete skeleton of a mammal that lived on the ancient supercontinent Gondwana, according to the magazine Nature.\"/>\n"
      "  </head>\n"
      "  <body>\n"
      "    <article>\n"
      "      <h1>Scientists Discover Potentially Oldest Mammal Dubbed 'Crazy Beast' in Madagascar</h1>\n"
      "      <h2>Researchers have discovered a nearly complete skeleton of a mammal that lived on the ancient supercontinent Gondwana, according to the magazine Nature.</h2>\n"
      "      <address>\n"
      "        <time datetime=\"2020-04-30T10:45:08+00:00\">30 Apr 2020, 10:45</time>\n"
      "      </address>\n"
      "      <p>Scientists from the US, Madagascar, and China led by David W. Krause have dug up an almost complete skeleton of an animal with an unusual surface on the first teeth located on the upper jaw behind fangs, the magazine <a href=\"https://www.nature.com/articles/s41586-020-2234-8\"><i>Nature</i> reported</a>. </p>\n"
      "      <p>The exquisitely preserved fossil discovered by the international research team has been named Adalatherium hui, which means \"crazy beast\".</p>\n"
      "      <blockquote>\"We could never have believed we would find such an extraordinary fossil of this mysterious mammal. This is the first real look at a novel experiment in mammal evolution\", one of the research team, evolutionary morphologist Alistair Evans from Monash University said.</blockquote>\n"
      "      <p>It is considered to be the most valuable find of a Mesozoic mammalian form.</p>\n"
      "      <p>According to the scientists, the small, cat-sized critter lived 66 million years ago during the Cretaceous Period and superficially resembled a badger with its long torso and stubby tail.</p>\n"
      "      <p>\"The Mesozoic history of the Mammalia is approximately twice as long as the Cenozoic. Because of their small size the remains of early mammals were often overlooked\", William A. Clemens, professor emeritus at the University of California at Berkeley said in his publication \"<a href=\"https://www.annualreviews.org/doi/pdf/10.1146/annurev.es.01.110170.002041\">Mesozoic Mammalian Evolution\".</a></p>\n"
      "    </article>\n"
      "  </body>\n"
      "</html>",
      to_iso_extended_string(t));
}

}  // namespace tgnews
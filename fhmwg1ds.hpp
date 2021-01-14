#include <string>
#include <vector>
#include <utility>
#include <cstdio>

namespace fhmwg {

struct LangStr {
    std::string text;
    std::string lang;
};

struct AltLang {
    std::vector<LangStr> entries;
    void dumpGEDCOM(FILE *, int);
    void dumpJSON(FILE *);
};

typedef std::string Date;
typedef std::string IRI;


struct Album {
    std::string name;
    IRI id;
    void dumpGEDCOM(FILE *);
    void dumpJSON(FILE *);
};

struct Location {
    double lat, lon;
    AltLang name;
    std::vector<IRI> ids;
    void dumpGEDCOM(FILE *);
    void dumpJSON(FILE *);
};

struct Region {
    enum Types { NONE=0, RECTANGLE, CIRCLE, POLYGON };
    Types type = Types::NONE;
    union {
        struct {double x, y, w, h;} rect;
        struct {double x, y, rx;} circ;
    };
    std::vector<std::pair<double, double>> pts;
    void dumpGEDCOM(FILE *, int);
    bool dumpJSON(FILE *, char);
};

struct Person {
    Region region;
    AltLang name;
    AltLang description;
    std::vector<IRI> ids;
    void dumpGEDCOM(FILE *);
    void dumpJSON(FILE *);
};

struct Object {
    Region region;
    AltLang title;
    void dumpGEDCOM(FILE *);
    void dumpJSON(FILE *);
};

struct ImageMetadata {
    AltLang title, caption, event;
    Date date;
    std::vector<Album> albums;
    std::vector<Location> locations;
    std::vector<Person> people;
    std::vector<Object> objects;
    void dumpGEDCOM(FILE *);
    void dumpJSON(FILE *, bool newlines=false);
    
    void parseFile(const char *filename);
};


/**
 * XML Namespaced used by FHMWG
 */
namespace ns {
    extern std::string dc, Iptc4xmpExt, mwg_coll, photoshop, rdf, exif, xml, xmp;
    extern const char *_dc, *_iptc, *_mwg, *_ph, *_rdf, *_exif, *_xml, *_xmp;
    void init();
}

} // namespace fhmwg

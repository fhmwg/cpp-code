#include "fhmwg1ds.hpp"
#include <cctype>
#include <cmath>

#define TXMP_STRING_TYPE	std::string
#define XMP_INCLUDE_XMPFILES 1
#include <XMP.hpp>
#include <XMP.incl_cpp>


namespace fhmwg {

/**
 * Edits a char* in place to whitespace normalize it: that is, strips
 * leading and trailing whitespace and collapses each other substring 
 * of whitespace with a single space character.
 * 
 * Returns the new length of the string.
 */
static size_t whitespaceNormalize(char *s) {
    size_t r = 0, w = 0;
    bool was = true;
    while(s[r]) {
        bool is = std::isspace(s[r]);
        if (is && !was) s[w++] = ' ';
        else if (!is) s[w++] = s[r];
        was = is;
        r += 1;
    }
    s[w] = 0;
    while(w > 0 && std::isspace(s[w-1]))
        s[--w] = 0;
    return w;
}

/**
 * Edits a std::string in place to whitespace normalize it: that is,
 * strips leading and trailing whitespace and collapses each other
 * substring of whitespace with a single space character.
 */
static void whitespaceNormalize(std::string& s) {
    s.resize(whitespaceNormalize(&s[0]));
}

namespace ns {
    std::string dc, Iptc4xmpExt, mwg_coll, photoshop, rdf, exif, xml, xmp;
    const char *_dc = "http://purl.org/dc/elements/1.1/",
        *_iptc = "http://iptc.org/std/Iptc4xmpExt/2008-02-29/",
        *_mwg = "http://www.metadataworkinggroup.com/schemas/collections/",
        *_ph = "http://ns.adobe.com/photoshop/1.0/",
        *_rdf = "http://www.w3.org/1999/02/22-rdf-syntax-ns#",
        *_exif = "http://ns.adobe.com/exif/1.0/",
        *_xml = "http://www.w3.org/XML/1998/namespace",
        *_xmp = "http://ns.adobe.com/xap/1.0/";
    void init() {
        SXMPMeta::GetNamespacePrefix(_dc, &dc)
        || SXMPMeta::RegisterNamespace(_dc, "dc", &dc);
        SXMPMeta::GetNamespacePrefix(_iptc, &Iptc4xmpExt)
        || SXMPMeta::RegisterNamespace(_iptc, "Iptc4xmpExt", &Iptc4xmpExt);
        SXMPMeta::GetNamespacePrefix(_mwg, &mwg_coll)
        || SXMPMeta::RegisterNamespace(_mwg, "mwg-coll", &mwg_coll);
        SXMPMeta::GetNamespacePrefix(_ph, &photoshop)
        || SXMPMeta::RegisterNamespace(_ph, "photoshop", &photoshop);
        SXMPMeta::GetNamespacePrefix(_rdf, &rdf)
        || SXMPMeta::RegisterNamespace(_rdf, "rdf", &rdf);
        SXMPMeta::GetNamespacePrefix(_exif, &exif)
        || SXMPMeta::RegisterNamespace(_exif, "exif", &exif);
        SXMPMeta::GetNamespacePrefix(_xml, &xml)
        || SXMPMeta::RegisterNamespace(_xml, "xml", &xml);
        SXMPMeta::GetNamespacePrefix(_xmp, &xmp)
        || SXMPMeta::RegisterNamespace(_xmp, "xmp", &xmp);
    }
}


/**
 * helper for LangString
 */
static void getArrayItemQuntifier(SXMPMeta xmpMeta,
XMP_StringPtr   schemaNS,
XMP_StringPtr   arrayName,
XMP_Index       itemIndex,
XMP_StringPtr   qualNS,
XMP_StringPtr   qualName,
std::string    *qualValue,
XMP_OptionBits *options) {
    std::string arrayPath;
    SXMPUtils::ComposeArrayItemPath(schemaNS, arrayName, itemIndex, &arrayPath);
    xmpMeta.GetQualifier(schemaNS, arrayPath.c_str(), qualNS, qualName, qualValue, options);
}

AltLang getAltLang(SXMPMeta xmp, const char *iri, const char *prop, bool normalize=false) {
    AltLang ans;
    LangStr tmp;
    XMP_OptionBits opt;
    if (!xmp.GetProperty(iri, prop, &tmp.text, &opt)) return ans;
    if (!(opt & kXMP_PropArrayIsAltText)) { // error, wrong type
        if (tmp.text.size() > 0) { // recoverable
            if (opt & kXMP_PropHasLang) {
                xmp.GetQualifier(iri, prop, ns::_xml, "xml:lang", &tmp.lang, 0);
            } else tmp.lang = "x-default";
            if (normalize) whitespaceNormalize(tmp.text);
            ans.entries.push_back(tmp);
        }
    } else { // AltLang
        XMP_Index num = xmp.CountArrayItems(iri, prop);
        for(int i=0; i<num; i+=1) {
            xmp.GetArrayItem(iri, prop, i+1, &tmp.text, &opt);
            getArrayItemQuntifier(xmp, iri, prop, i+1, ns::_xml, "xml:lang", &tmp.lang, 0);
            if (normalize) whitespaceNormalize(tmp.text);
            ans.entries.push_back(tmp);
        }
    }
    return ans;
}

std::string getLineText(SXMPMeta xmp, const char *iri, const char *prop) {
    std::string ans;
    xmp.GetProperty(iri, prop, &ans, 0);
    return ans;
}

std::vector<IRI> getIDs(SXMPMeta xmp, const char *iri, const char *prop) {
    std::vector<IRI> ans;
    IRI tmp;
    XMP_OptionBits opt;
    if (!xmp.GetProperty(iri, prop, &tmp, &opt)) return ans;
    if (opt & kXMP_PropArrayIsAltText) return ans;
    if (opt & kXMP_PropValueIsArray || opt & kXMP_PropArrayIsOrdered || opt & kXMP_PropArrayIsAlternate) {
        XMP_Index num = xmp.CountArrayItems(iri, prop);
        for(int i=0; i<num; i+=1) {
            xmp.GetArrayItem(iri, prop, i+1, &tmp, &opt);
            whitespaceNormalize(tmp);
            ans.push_back(tmp);
        }
    } else {
        whitespaceNormalize(tmp);
        ans.push_back(tmp);
    }
    return ans;
}

std::vector<Location> getLocations(SXMPMeta xmp) {
    std::vector<Location> ans;
    XMP_Index num = xmp.CountArrayItems(ns::_iptc, "LocationShown");
    for(int i=0; i<num; i+=1) {
        Location tmp;

        std::string cell, prop;
        SXMPUtils::ComposeArrayItemPath(ns::_iptc, "LocationShown", i+1, &cell);
        SXMPUtils::ComposeStructFieldPath(ns::_iptc, cell.c_str(), ns::_iptc, "LocationName", &prop);
        tmp.name = getAltLang(xmp, ns::_iptc, prop.c_str());
        if (tmp.name.entries.size() == 0) {
            // assemble a name from other parts
            LangStr built;
            built.lang = "x-default";
            
            SXMPUtils::ComposeStructFieldPath(ns::_iptc, cell.c_str(), ns::_iptc, "Sublocation", &prop);
            AltLang sub = getAltLang(xmp, ns::_iptc, prop.c_str());
            if (sub.entries.size() > 0) built.text = sub.entries[0].text;
            
            SXMPUtils::ComposeStructFieldPath(ns::_iptc, cell.c_str(), ns::_iptc, "City", &prop);
            AltLang city = getAltLang(xmp, ns::_iptc, prop.c_str());
            if (city.entries.size() > 0) {
                if (built.text.size() > 0) built.text += ", ";
                built.text += city.entries[0].text;
            }
            
            SXMPUtils::ComposeStructFieldPath(ns::_iptc, cell.c_str(), ns::_iptc, "ProvinceState", &prop);
            AltLang state = getAltLang(xmp, ns::_iptc, prop.c_str());
            if (state.entries.size() > 0) {
                if (built.text.size() > 0) built.text += ", ";
                built.text += state.entries[0].text;
            }

            SXMPUtils::ComposeStructFieldPath(ns::_iptc, cell.c_str(), ns::_iptc, "CountryName", &prop);
            AltLang country = getAltLang(xmp, ns::_iptc, prop.c_str());
            if (country.entries.size() == 0) {
                SXMPUtils::ComposeStructFieldPath(ns::_iptc, cell.c_str(), ns::_iptc, "CountryCode", &prop);
                country = getAltLang(xmp, ns::_iptc, prop.c_str());
            }
            if (country.entries.size() > 0) {
                if (built.text.size() > 0) built.text += ", ";
                built.text += country.entries[0].text;
            }
            
            SXMPUtils::ComposeStructFieldPath(ns::_iptc, cell.c_str(), ns::_iptc, "WorldRegion", &prop);
            AltLang region = getAltLang(xmp, ns::_iptc, prop.c_str());
            if (region.entries.size() > 0) {
                if (built.text.size() > 0) built.text += ", ";
                built.text += region.entries[0].text;
            }

            if (built.text.size() > 0)
                tmp.name.entries.push_back(built);
        }
        
        SXMPUtils::ComposeStructFieldPath(ns::_iptc, cell.c_str(), ns::_exif, "GPSLatitude", &prop);
        if (!xmp.GetProperty_Float(ns::_iptc, prop.c_str(), &tmp.lat, 0)) tmp.lat = NAN;
        SXMPUtils::ComposeStructFieldPath(ns::_iptc, cell.c_str(), ns::_exif, "GPSLongitutde", &prop);
        if (!xmp.GetProperty_Float(ns::_iptc, prop.c_str(), &tmp.lon, 0)) tmp.lon = NAN;
        
        SXMPUtils::ComposeStructFieldPath(ns::_iptc, cell.c_str(), ns::_exif, "LocationId", &prop);
        tmp.ids = getIDs(xmp, ns::_iptc, prop.c_str());
        
        ans.push_back(tmp);
    }
    return ans;
}

std::vector<Album> getAlbums(SXMPMeta xmp) {
    std::vector<Album> ans;
    XMP_Index num = xmp.CountArrayItems(ns::_mwg, "Collections");
    for(int i=0; i<num; i+=1) {
        Album tmp;

        std::string cell, prop;
        SXMPUtils::ComposeArrayItemPath(ns::_mwg, "Collections", i+1, &cell);
        SXMPUtils::ComposeStructFieldPath(ns::_mwg, cell.c_str(), ns::_mwg, "CollectionName", &prop);
        xmp.GetProperty(ns::_mwg, prop.c_str(), &tmp.name, 0);
        SXMPUtils::ComposeStructFieldPath(ns::_mwg, cell.c_str(), ns::_mwg, "CollectionURI", &prop);
        xmp.GetProperty(ns::_mwg, prop.c_str(), &tmp.id, 0);
        
        if (tmp.name.size() > 0 || tmp.id.size() > 0)
            ans.push_back(tmp);
    }
    return ans;
}

/** Given path to a PersonInImageWDetails, add those people to out */
static void processPeople(SXMPMeta xmp, const char *piiwd, const Region& region, std::vector<Person>& out) {
    XMP_Index num = xmp.CountArrayItems(ns::_iptc, piiwd);
    for(int i=0; i<num; i+=1) {
        Person tmp;
        tmp.region = region;
        
        std::string cell, prop;
        SXMPUtils::ComposeArrayItemPath(ns::_iptc, piiwd, i+1, &cell);
        SXMPUtils::ComposeStructFieldPath(ns::_mwg, cell.c_str(), ns::_iptc, "PersonName", &prop);
        tmp.name = getAltLang(xmp, ns::_iptc, prop.c_str(), true);
        SXMPUtils::ComposeStructFieldPath(ns::_mwg, cell.c_str(), ns::_iptc, "PersonDescription", &prop);
        tmp.description = getAltLang(xmp, ns::_iptc, prop.c_str());
        SXMPUtils::ComposeStructFieldPath(ns::_mwg, cell.c_str(), ns::_iptc, "PersonId", &prop);
        tmp.ids = getIDs(xmp, ns::_iptc, prop.c_str());
        
        out.push_back(tmp);
    }
}

/** Given path to a PersonInImage, add those people to out */
static void processSimplePeople(SXMPMeta xmp, const char *pii, const Region& region, std::vector<Person>& out) {
    XMP_Index num = xmp.CountArrayItems(ns::_iptc, pii);
    for(int i=0; i<num; i+=1) {
        Person tmp;
        tmp.region = region;
        
        std::string prop;
        SXMPUtils::ComposeArrayItemPath(ns::_iptc, pii, i+1, &prop);
        tmp.name = getAltLang(xmp, ns::_iptc, prop.c_str(), true);
        
        out.push_back(tmp);
    }
}

/** Given path to a ArtworkOrObject, add those objects to out */
static void processObjects(SXMPMeta xmp, const char *aoo, const Region& region, std::vector<Object>& out) {
    XMP_Index num = xmp.CountArrayItems(ns::_iptc, aoo);
    for(int i=0; i<num; i+=1) {
        Object tmp;
        tmp.region = region;
        
        std::string cell, prop;
        SXMPUtils::ComposeArrayItemPath(ns::_iptc, aoo, i+1, &cell);
        SXMPUtils::ComposeStructFieldPath(ns::_mwg, cell.c_str(), ns::_iptc, "AOTitle", &prop);
        tmp.title = getAltLang(xmp, ns::_iptc, prop.c_str());
        
        out.push_back(tmp);
    }
}

static Region getRegionOf(SXMPMeta xmp, const char *cell) {
    Region ans; ans.type = Region::Types::NONE;
    std::string thing, item, val;
    SXMPUtils::ComposeStructFieldPath(ns::_iptc, cell, ns::_iptc, "RegionBoundary", &thing);
    SXMPUtils::ComposeStructFieldPath(ns::_iptc, thing.c_str(), ns::_iptc, "rbUnit", &item);
    xmp.GetProperty(ns::_iptc, item.c_str(), &val, 0);
    if (val != "relative") return ans; // Future Feature: add pixel-to-relative conversion
    SXMPUtils::ComposeStructFieldPath(ns::_iptc, thing.c_str(), ns::_iptc, "rbShape", &item);
    xmp.GetProperty(ns::_iptc, item.c_str(), &val, 0);
    if (val == "circle") {
        ans.type = Region::Types::CIRCLE;
        SXMPUtils::ComposeStructFieldPath(ns::_iptc, thing.c_str(), ns::_iptc, "rbX", &item);
        xmp.GetProperty_Float(ns::_iptc, item.c_str(), &ans.circ.x, 0);
        SXMPUtils::ComposeStructFieldPath(ns::_iptc, thing.c_str(), ns::_iptc, "rbY", &item);
        xmp.GetProperty_Float(ns::_iptc, item.c_str(), &ans.circ.y, 0);
        SXMPUtils::ComposeStructFieldPath(ns::_iptc, thing.c_str(), ns::_iptc, "rbRx", &item);
        xmp.GetProperty_Float(ns::_iptc, item.c_str(), &ans.circ.rx, 0);
    } else if (val == "rectangle") {
        ans.type = Region::Types::RECTANGLE;
        SXMPUtils::ComposeStructFieldPath(ns::_iptc, thing.c_str(), ns::_iptc, "rbX", &item);
        xmp.GetProperty_Float(ns::_iptc, item.c_str(), &ans.rect.x, 0);
        SXMPUtils::ComposeStructFieldPath(ns::_iptc, thing.c_str(), ns::_iptc, "rbY", &item);
        xmp.GetProperty_Float(ns::_iptc, item.c_str(), &ans.rect.y, 0);
        SXMPUtils::ComposeStructFieldPath(ns::_iptc, thing.c_str(), ns::_iptc, "rbW", &item);
        xmp.GetProperty_Float(ns::_iptc, item.c_str(), &ans.rect.w, 0);
        SXMPUtils::ComposeStructFieldPath(ns::_iptc, thing.c_str(), ns::_iptc, "rbH", &item);
        xmp.GetProperty_Float(ns::_iptc, item.c_str(), &ans.rect.h, 0);
        if (ans.rect.w == 0 && ans.rect.h == 0 && ans.rect.w == 1 && ans.rect.h == 1)
            ans.type = Region::Types::NONE;
    } else if (val == "polygon") {
        ans.type = Region::Types::POLYGON;
        SXMPUtils::ComposeStructFieldPath(ns::_iptc, thing.c_str(), ns::_iptc, "rbVertices", &item);
        XMP_Index num = xmp.CountArrayItems(ns::_iptc, item.c_str());
        for(int i=0; i<num; i+=1) {
            std::string vert, num;
            double x,y;
            SXMPUtils::ComposeArrayItemPath(ns::_iptc, item.c_str(), i+1, &vert);
            SXMPUtils::ComposeStructFieldPath(ns::_iptc, vert.c_str(), ns::_iptc, "rbX", &num);
            xmp.GetProperty_Float(ns::_iptc, num.c_str(), &x, 0);
            SXMPUtils::ComposeStructFieldPath(ns::_iptc, vert.c_str(), ns::_iptc, "rbY", &num);
            xmp.GetProperty_Float(ns::_iptc, num.c_str(), &y, 0);
            ans.pts.push_back(std::pair<double,double>(x,y));
        }
    }
    return ans;
}

void ImageMetadata::parseFile(const char *fileName) {
	bool ok;

	SXMPMeta  xmpMeta;	
	SXMPFiles xmpFile;
	XMP_FileFormat format;
	XMP_OptionBits openFlags, handlerFlags;
	XMP_PacketInfo xmpPacket;
	
	xmpFile.OpenFile ( fileName, kXMP_UnknownFile, kXMPFiles_OpenForRead );
	ok = xmpFile.GetFileInfo ( 0, &openFlags, &format, &handlerFlags );
	if ( ! ok ) return;

	ok = xmpFile.GetXMP ( &xmpMeta, 0, &xmpPacket );
	if ( ! ok ) return;
    
#ifdef DUMP_EVERYTHING   
    SXMPIterator it = SXMPIterator(xmpMeta, 0, 0, 0);
    std::string sna, path, val; XMP_OptionBits opt;
    while(it.Next(&sna, &path, &val, &opt)) {
        if (path.size() > 0)
        fprintf(stdout,"%s :: %s = %s\n", sna.c_str(), path.c_str(), val.c_str());
    }
#endif

    // first the simple ones: values or AltLang text directly in root

    this->title = getAltLang(xmpMeta, ns::_dc, "title", true);
    if (this->title.entries.size() == 0)
        this->title = getAltLang(xmpMeta, ns::_ph, "Headline", true);
    
    this->caption = getAltLang(xmpMeta, ns::_dc, "description");
    // no XMP defaults if missing
    
    this->event = getAltLang(xmpMeta, ns::_iptc, "Event");
    // no XMP defaults if missing

    this->date = getLineText(xmpMeta, ns::_ph, "DateCreated");
    if (this->date.size() == 0)
        this->date = getLineText(xmpMeta, ns::_exif, "DateTimeOriginal");
    if (this->date.size() == 0)
        this->date = getLineText(xmpMeta, ns::_dc, "date");
    if (this->date.size() == 0)
        this->date = getLineText(xmpMeta, ns::_exif, "DateTimeDigitized");
    if (this->date.size() == 0)
        this->date = getLineText(xmpMeta, ns::_xmp, "CreateDate");
    if (this->date.size() == 0)
        this->date = getLineText(xmpMeta, ns::_exif, "DateTime");
    if (this->date.size() == 0)
        this->date = getLineText(xmpMeta, ns::_xmp, "ModifyDate");
    if (this->date.size() == 0)
        this->date = getLineText(xmpMeta, ns::_xmp, "MetadataDate");

    // then the medium complexity: structs directly in root

    this->locations = getLocations(xmpMeta);
    this->albums = getAlbums(xmpMeta);
    
    // then the complex ones: regioned data
    // first those not covered by FHMWG: those not inside any region
    Region region; region.type = Region::Types::NONE;
    processPeople(xmpMeta, "PersonInImageWDetails", region, this->people);
    processSimplePeople(xmpMeta, "PersonInImage", region, this->people);
    processObjects(xmpMeta, "ArtworkOrObject", region, this->objects);
    // then those inside regions
    XMP_Index regions = xmpMeta.CountArrayItems(ns::_iptc, "ImageRegion");
    for(int i=0; i<regions; i+=1) {
        std::string cell, path;
        SXMPUtils::ComposeArrayItemPath(ns::_iptc, "ImageRegion", i+1, &cell);
        region = getRegionOf(xmpMeta, cell.c_str());

        SXMPUtils::ComposeStructFieldPath(ns::_iptc, cell.c_str(), ns::_iptc, "PersonInImageWDetails", &path);
        processPeople(xmpMeta, path.c_str(), region, this->people);
        SXMPUtils::ComposeStructFieldPath(ns::_iptc, cell.c_str(), ns::_iptc, "PersonInImage", &path);
        processSimplePeople(xmpMeta, path.c_str(), region, this->people);
        SXMPUtils::ComposeStructFieldPath(ns::_iptc, cell.c_str(), ns::_iptc, "ArtworkOrObject", &path);
        processObjects(xmpMeta, path.c_str(), region, this->objects);
    }
    
	xmpFile.CloseFile();
}



} // namespace fhmwg


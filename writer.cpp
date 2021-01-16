#include "fhmwg1ds.hpp"
#include "json.hpp"

#define TXMP_STRING_TYPE	std::string
#define XMP_INCLUDE_XMPFILES 1
#include <XMP.hpp>
#include <XMP.incl_cpp>

#include <unistd.h>
#include <fcntl.h>

using json = nlohmann::ordered_json;

namespace fhmwg {

/**
 * Special handling to bypass an oddity with the toolkit.
 * XMP says
 * 
 * - if known, default MUST be first
 * - if present, "x-default" MUST be default
 * - if present and not alone, "x-default" SHOULD have languaged copy
 * 
 * but the toolkit pretends that second SHOULD is a MUST and using
 * SetLocalizedText forces x-default to match the first non-x-default
 * language, changing the value of the first added to enforce this.
 * 
 * This function is a work-around to support the optional nature of the
 * SHOULD rule above.
 */
static void setAltLang(SXMPMeta xmp, const char *iri, const char *prop, json altLang) {
    if (altLang.is_string())
        altLang = json::object({{"x-default",altLang}});
    for(auto& pair : altLang.items()) {
        /* naive:
         * xmp.SetLocalizedText(iri, prop, 0, pair.key().c_str(), pair.value(), 0);
         */
        // work-around: manual assignment of array entries
        xmp.AppendArrayItem(iri, prop, kXMP_PropArrayIsAltText, pair.value(), 0);
        std::string item;
        SXMPUtils::ComposeArrayItemPath(iri, prop, xmp.CountArrayItems(iri, prop), &item);
        xmp.SetQualifier(iri, item.c_str(), ns::_xml, "lang", pair.key(), 0);
    }
}

/**
 * Works directly on JSON instead of on ImageMetadata because we want
 * a missing key to be ignored, while an empty key removes.
 */
void updateMetadata(SXMPMeta xmp, json j) {
    if (j.contains("caption")) {
        xmp.DeleteProperty(ns::_dc, "description");
        if (j["caption"].is_object() || j["caption"].is_string())
            setAltLang(xmp, ns::_dc, "description", j["caption"]);
    }
    if (j.contains("title")) {
        xmp.DeleteProperty(ns::_dc, "title");
        if (j["title"].is_object() || j["title"].is_string())
            setAltLang(xmp, ns::_dc, "title", j["title"]);
    }
    if (j.contains("event")) {
        xmp.DeleteProperty(ns::_iptc, "Event");
        if (j["event"].is_object() || j["event"].is_string())
            setAltLang(xmp, ns::_iptc, "Event", j["event"]);
    }
    if (j.contains("date")) {
        xmp.DeleteProperty(ns::_ph, "DateCreated");
        if (j["date"].is_string())
            xmp.SetProperty(ns::_ph, "DateCreated", j["date"], 0);
    }
    
    if (j.contains("albums")) {
        xmp.DeleteProperty(ns::_mwg, "Collections");
        if (j["albums"].is_array()) {
            xmp.SetProperty(ns::_mwg, "Collections", 0, kXMP_PropValueIsArray);
            for (auto& a : j["albums"]) {
                xmp.AppendArrayItem(ns::_mwg, "Collections", 0, 0, kXMP_PropValueIsStruct);
                std::string album;
                SXMPUtils::ComposeArrayItemPath(ns::_mwg, "Collections", xmp.CountArrayItems(ns::_mwg, "Collections"), &album);
                std::string entry;
                if (a.contains("name")) {
                    SXMPUtils::ComposeStructFieldPath(ns::_mwg, album.c_str(), ns::_mwg, "CollectionName", &entry);
                    xmp.SetProperty(ns::_mwg, entry.c_str(), a["name"], 0);
                }
                if (a.contains("id")) {
                    SXMPUtils::ComposeStructFieldPath(ns::_mwg, album.c_str(), ns::_mwg, "CollectionURI", &entry);
                    xmp.SetProperty(ns::_mwg, entry.c_str(), a["id"], 0);
                }
            }
        }
    }

    if (j.contains("locations")) {
        xmp.DeleteProperty(ns::_iptc, "LocationShown");
        if (j["locations"].is_array()) {
            xmp.SetProperty(ns::_iptc, "LocationShown", 0, kXMP_PropValueIsArray);
            for (auto& a : j["locations"]) {
                xmp.AppendArrayItem(ns::_iptc, "LocationShown", 0, 0, kXMP_PropValueIsStruct);
                std::string loc;
                SXMPUtils::ComposeArrayItemPath(ns::_iptc, "LocationShown", xmp.CountArrayItems(ns::_iptc, "LocationShown"), &loc);
                std::string entry;
                if (a.contains("name")) {
                    SXMPUtils::ComposeStructFieldPath(ns::_iptc, loc.c_str(), ns::_iptc, "LocationName", &entry);
                    setAltLang(xmp, ns::_iptc, entry.c_str(), a["name"]);
                }
                if (a.contains("latitude") && a.contains("longitude")) {
                    SXMPUtils::ComposeStructFieldPath(ns::_iptc, loc.c_str(), ns::_exif, "GPSLatitude", &entry);
                    xmp.SetProperty_Float(ns::_iptc, entry.c_str(), a["latitude"], 0);
                    SXMPUtils::ComposeStructFieldPath(ns::_iptc, loc.c_str(), ns::_exif, "GPSLongitude", &entry);
                    xmp.SetProperty_Float(ns::_iptc, entry.c_str(), a["longitude"], 0);
                }
                if (a.contains("ids")) {
                    SXMPUtils::ComposeStructFieldPath(ns::_iptc, loc.c_str(), ns::_iptc, "LocationId", &entry);
                    for(auto& iri : a["ids"])
                        xmp.AppendArrayItem(ns::_iptc, entry.c_str(), 0, iri, 0);
                }
            }
        }
    }

    #warning "TO DO: handle people"
    #warning "TO DO: handle objects"
}

bool copyFile(const char *from, const char *to) {
    char buffer[4096];
    int r = open(from, O_RDONLY);
    if (r < 0) return false;
    int w = open(to, O_CREAT | O_EXCL | O_WRONLY, 0644);
    if (w < 0) { close(r); return false; }
    ssize_t got;
    while ((got = read(r, buffer, sizeof(buffer))) > 0)
        write(w, buffer, got);
    close(r);
    close(w);
    return true;
}

} // namespace fhmwg

int main(int argc, char *argv[]) {
    
    if (argc != 3
    || access(argv[1], R_OK) != 0
    || access(argv[2], F_OK) == 0
    ) {
        fprintf(stdout, "USAGE: %s inputimage outputimage\n    inputimage must exist and be an image file\n    outputimage must not exist\n    metadata to edit is provided as a JSON object on stdin\n", argv[0]);
        return -1;
    }
    
    if (!SXMPMeta::Initialize()) {
		fprintf(stderr, "## SXMPMeta::Initialize failed!\n");
		return -1;
	}	

    fhmwg::ns::init();
    
    SXMPMeta  xmpMeta;
	SXMPFiles file;

    if (!fhmwg::copyFile(argv[1], argv[2])) {
        fprintf(stderr, "Failed to create \"%s\"\n", argv[2]);
        return -1;
    }
	
	if (!file.OpenFile ( argv[2], kXMP_UnknownFile, kXMPFiles_OpenForUpdate )
    || !file.GetXMP ( &xmpMeta, 0, 0 )) {
        fprintf(stderr, "Failed to open and parse \"%s\"\n", argv[1]);
        return -1;
    }

    auto j = json::parse(stdin);
    fhmwg::updateMetadata(xmpMeta, j);

    file.PutXMP(xmpMeta);
    file.CloseFile();
		
	SXMPFiles::Terminate();
	SXMPMeta::Terminate();

    return 0;
}

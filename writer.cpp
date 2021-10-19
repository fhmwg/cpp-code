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
	
	// Complicated workaround.
	
	// 1. Find the x-default, if any
	std::string def;
	for(auto& pair : altLang.items()) {
		if (pair.key() == "x-default") def = pair.value();
	}
	
	if (def.length() == 0) {
		// 2. If no x-default, no workaround needed
		for(auto& pair : altLang.items()) {
			xmp.SetLocalizedText(iri, prop, NULL, pair.key().c_str(), pair.value(), 0);
		}
	} else {
		// 3. Put the x-default first
		xmp.SetLocalizedText(iri, prop, NULL, "x-default", def, 0);
		std::string defLang;
		// 4. And then its language-tagged copy, if any
		bool other = false;
		for(auto& pair : altLang.items()) {
			if (pair.value() == def && pair.key() != "x-default") {
				xmp.SetLocalizedText(iri, prop, NULL, pair.key().c_str(), pair.value(), 0);
				defLang = pair.key();
				break;
			} else { other = true; }
		}
		if (other && defLang.length() == 0) {
			// 5. If no language-tagged copy, make a language-tagged copy with tag `und`
			xmp.SetLocalizedText(iri, prop, NULL, "und", def, 0);
		}
		// 6. then put all the non-default languages
		for(auto& pair : altLang.items()) {
			if (pair.key() != defLang && pair.key() != "x-default") {
				xmp.SetLocalizedText(iri, prop, NULL, pair.key().c_str(), pair.value().get<std::string>(), 0);
			}
		}
	}
}

static void setRegionArea(SXMPMeta xmp, const char *prop, const json& object) {
	std::string s_path, item;
	SXMPUtils::ComposeStructFieldPath(ns::_iptc, prop, ns::_iptc, "RegionBoundary", &s_path);
	const char *path = s_path.c_str();
	xmp.SetProperty(ns::_iptc, path, 0, kXMP_PropValueIsStruct);
	xmp.SetStructField(ns::_iptc, path, ns::_iptc, "rbUnit", "relative", 0);
	if (object.contains("circle")) {
		xmp.SetStructField(ns::_iptc, path, ns::_iptc, "rbShape", "circle", 0);
		SXMPUtils::ComposeStructFieldPath(ns::_iptc, path, ns::_iptc, "rbX", &item);
		xmp.SetProperty_Float(ns::_iptc, item.c_str(), object["circle"]["x"], 0);
		SXMPUtils::ComposeStructFieldPath(ns::_iptc, path, ns::_iptc, "rbY", &item);
		xmp.SetProperty_Float(ns::_iptc, item.c_str(), object["circle"]["y"], 0);
		SXMPUtils::ComposeStructFieldPath(ns::_iptc, path, ns::_iptc, "rbRx", &item);
		xmp.SetProperty_Float(ns::_iptc, item.c_str(), object["circle"]["rx"], 0);
	} else if (object.contains("rectangle")) {
		xmp.SetStructField(ns::_iptc, path, ns::_iptc, "rbShape", "rectangle", 0);
		SXMPUtils::ComposeStructFieldPath(ns::_iptc, path, ns::_iptc, "rbX", &item);
		xmp.SetProperty_Float(ns::_iptc, item.c_str(), object["rectangle"]["x"], 0);
		SXMPUtils::ComposeStructFieldPath(ns::_iptc, path, ns::_iptc, "rbY", &item);
		xmp.SetProperty_Float(ns::_iptc, item.c_str(), object["rectangle"]["y"], 0);
		SXMPUtils::ComposeStructFieldPath(ns::_iptc, path, ns::_iptc, "rbW", &item);
		xmp.SetProperty_Float(ns::_iptc, item.c_str(), object["rectangle"]["w"], 0);
		SXMPUtils::ComposeStructFieldPath(ns::_iptc, path, ns::_iptc, "rbH", &item);
		xmp.SetProperty_Float(ns::_iptc, item.c_str(), object["rectangle"]["h"], 0);
	} else if (object.contains("polygon")) {
		xmp.SetStructField(ns::_iptc, path, ns::_iptc, "rbShape", "polygon", 0);
		SXMPUtils::ComposeStructFieldPath(ns::_iptc, path, ns::_iptc, "rbVertices", &item);
		for(auto& pt : object["polygon"]) {
			std::string point, coord;
			xmp.AppendArrayItem(ns::_iptc, item.c_str(), kXMP_PropArrayIsOrdered, 0, kXMP_PropValueIsStruct);
			SXMPUtils::ComposeArrayItemPath(ns::_iptc, item.c_str(), xmp.CountArrayItems(ns::_iptc, item.c_str()), &point);
			SXMPUtils::ComposeStructFieldPath(ns::_iptc, point.c_str(), ns::_iptc, "rbX", &coord);
			xmp.SetProperty_Float(ns::_iptc, coord.c_str(), pt["x"], 0);
			SXMPUtils::ComposeStructFieldPath(ns::_iptc, point.c_str(), ns::_iptc, "rbY", &coord);
			xmp.SetProperty_Float(ns::_iptc, coord.c_str(), pt["y"], 0);
		}
	} else {
		// use the whole-image region
		xmp.SetStructField(ns::_iptc, path, ns::_iptc, "rbShape", "rectangle", 0);
		xmp.SetStructField(ns::_iptc, path, ns::_iptc, "rbX", "0", 0);
		xmp.SetStructField(ns::_iptc, path, ns::_iptc, "rbY", "0", 0);
		xmp.SetStructField(ns::_iptc, path, ns::_iptc, "rbW", "1", 0);
		xmp.SetStructField(ns::_iptc, path, ns::_iptc, "rbH", "1", 0);
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
					for(const std::string& iri : a["ids"]) {
						xmp.AppendArrayItem(ns::_iptc, entry.c_str(), kXMP_PropValueIsArray, iri.c_str(), 0);
					}
				}
			}
		}
	}

	if (j.contains("people")) {
		// remove all PersonInImage and PersonInImageWDetails, both at top level and in regions. If this renders a region empty, also remove the region.
		xmp.DeleteProperty(ns::_iptc, "PersonInImage");
		xmp.DeleteProperty(ns::_iptc, "PersonInImageWDetails");
		XMP_Index regions = xmp.CountArrayItems(ns::_iptc, "ImageRegion");
		
		// iterate backwards so that removing regions does not re-index yet-to-be-visited regions
		for(int i=regions; i>0; i-=1) {
			// remove people from this region
			std::string cell, path;
			SXMPUtils::ComposeArrayItemPath(ns::_iptc, "ImageRegion", i, &cell);
			SXMPUtils::ComposeStructFieldPath(ns::_iptc, cell.c_str(), ns::_iptc, "PersonInImageWDetails", &path);
			xmp.DeleteProperty(ns::_iptc, path.c_str());
			SXMPUtils::ComposeStructFieldPath(ns::_iptc, cell.c_str(), ns::_iptc, "PersonInImage", &path);
			xmp.DeleteProperty(ns::_iptc, path.c_str());
			
			// and if the region is now empty, remove the region too
			auto iter = SXMPIterator (xmp, ns::_iptc, cell.c_str(), kXMP_IterJustChildren | kXMP_IterOmitQualifiers);
			std::string kpath;
			bool empty = true;
			while(iter.Next(0, &kpath, 0, 0))
				if(kpath.find("/Iptc4xmpExt:RegionBoundary") != std::string::npos) { empty = false; break; }
			if (empty)
				xmp.DeleteArrayItem(ns::_iptc, "ImageRegion", i);
		}
		
		// add a region with a PersonInImageWDetails[1] for each person
		for(auto& person : j["people"]) {
			// add a region
			xmp.AppendArrayItem(ns::_iptc, "ImageRegion", kXMP_PropValueIsArray, 0, kXMP_PropValueIsStruct);
			std::string cell, path, item, entry;
			SXMPUtils::ComposeArrayItemPath(ns::_iptc, "ImageRegion", xmp.CountArrayItems(ns::_iptc, "ImageRegion"), &cell);
			
			// add a struct with an area to that region
			xmp.SetProperty(ns::_iptc, cell.c_str(), 0, kXMP_PropValueIsStruct);
			setRegionArea(xmp, cell.c_str(), person);
			// add a person array with one person in it to that struct
			SXMPUtils::ComposeStructFieldPath(ns::_iptc, cell.c_str(), ns::_iptc, "PersonInImageWDetails", &path);
			xmp.AppendArrayItem(ns::_iptc, path.c_str(), kXMP_PropValueIsArray, 0, kXMP_PropValueIsStruct);
			SXMPUtils::ComposeArrayItemPath(ns::_iptc, path.c_str(), 1, &item);
			// add details about the person to that array item
			if (person.contains("name")) {
				SXMPUtils::ComposeStructFieldPath(ns::_iptc, item.c_str(), ns::_iptc, "PersonName", &entry);
				setAltLang(xmp, ns::_iptc, entry.c_str(), person["name"]);
			}
			if (person.contains("description")) {
				SXMPUtils::ComposeStructFieldPath(ns::_iptc, item.c_str(), ns::_iptc, "PersonDescription", &entry);
				setAltLang(xmp, ns::_iptc, entry.c_str(), person["description"]);
			}
			if (person.contains("ids")) {
				SXMPUtils::ComposeStructFieldPath(ns::_iptc, item.c_str(), ns::_iptc, "PersonId", &entry);
				for(const std::string& iri : person["ids"]) {
					xmp.AppendArrayItem(ns::_iptc, entry.c_str(), kXMP_PropValueIsArray, iri.c_str(), 0);
				}
			}
		}
	}

	if (j.contains("objects")) {
		// remove all ArtworkOrObject, both at top level and in regions. If this renders a region empty, also remove the region.
		xmp.DeleteProperty(ns::_iptc, "ArtworkOrObject");
		XMP_Index regions = xmp.CountArrayItems(ns::_iptc, "ImageRegion");
		
		// iterate backwards so that removing regions does not re-index yet-to-be-visited regions
		for(int i=regions; i>0; i-=1) {
			// remove people from this region
			std::string cell, path;
			SXMPUtils::ComposeArrayItemPath(ns::_iptc, "ImageRegion", i, &cell);
			SXMPUtils::ComposeStructFieldPath(ns::_iptc, cell.c_str(), ns::_iptc, "ArtworkOrObject", &path);
			xmp.DeleteProperty(ns::_iptc, path.c_str());
			
			// and if the region is now empty, remove the region too
			auto iter = SXMPIterator (xmp, ns::_iptc, cell.c_str(), kXMP_IterJustChildren | kXMP_IterOmitQualifiers);
			std::string kpath;
			bool empty = true;
			while(iter.Next(0, &kpath, 0, 0))
				if(kpath.find("/Iptc4xmpExt:RegionBoundary") != std::string::npos) { empty = false; break; }
			if (empty)
				xmp.DeleteArrayItem(ns::_iptc, "ImageRegion", i);
		}
		
		// add a region with a ArtworkOrObject[1] for each object
		for(auto& object : j["objects"]) {
			// add a region
			xmp.AppendArrayItem(ns::_iptc, "ImageRegion", kXMP_PropValueIsArray, 0, kXMP_PropValueIsStruct);
			std::string cell, path, item, entry;
			SXMPUtils::ComposeArrayItemPath(ns::_iptc, "ImageRegion", xmp.CountArrayItems(ns::_iptc, "ImageRegion"), &cell);
			
			// add a struct with an area to that region
			xmp.SetProperty(ns::_iptc, cell.c_str(), 0, kXMP_PropValueIsStruct);
			setRegionArea(xmp, cell.c_str(), object);
			// add a person array with one person in it to that struct
			SXMPUtils::ComposeStructFieldPath(ns::_iptc, cell.c_str(), ns::_iptc, "ArtworkOrObject", &path);
			xmp.AppendArrayItem(ns::_iptc, path.c_str(), kXMP_PropValueIsArray, 0, kXMP_PropValueIsStruct);
			SXMPUtils::ComposeArrayItemPath(ns::_iptc, path.c_str(), 1, &item);
			// add details about the person to that array item
			if (object.contains("title")) {
				SXMPUtils::ComposeStructFieldPath(ns::_iptc, item.c_str(), ns::_iptc, "AOTitle", &entry);
				setAltLang(xmp, ns::_iptc, entry.c_str(), object["title"]);
			}
		}
	}

}

bool copyFile(const char *from, const char *to) {
	char buffer[4096];
	int r = open(from, O_RDONLY);
	if (r < 0) return false;
	int w = open(to, O_CREAT | O_EXCL | O_WRONLY, 0644);
	if (w < 0) { close(r); return false; }
	ssize_t got;
	while ((got = read(r, buffer, sizeof(buffer))) > 0) {
		ssize_t sofar = 0;
		while(sofar < got) {
			ssize_t wrote = write(w, buffer, got);
			if (wrote < 0) return false;
			sofar += wrote;
		}
	}
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
	try {
		fhmwg::updateMetadata(xmpMeta, j);
	} catch (XMP_Error ex) {
		fprintf(stderr, "CRASHED with error %d:\n  %s\n", ex.GetID(), ex.GetErrMsg());
		throw ex;
	}

	file.PutXMP(xmpMeta);
	file.CloseFile();
		
	SXMPFiles::Terminate();
	SXMPMeta::Terminate();

	return 0;
}

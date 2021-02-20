#include "fhmwg1ds.hpp"
#define TXMP_STRING_TYPE	std::string
#define XMP_INCLUDE_XMPFILES 1
#include <XMP.hpp>
#include <XMP.incl_cpp>
#include <cstring>

int main(int argc, char *argv[]) {
    if (!SXMPMeta::Initialize()) {
		fprintf(stderr, "## SXMPMeta::Initialize failed!\n");
		return -1;
	}	

    fhmwg::ns::init();
    
    bool asGEDCOM = false;

	for (int i = 1; i < argc; ++i) {
        if (!strcmp("-g", argv[i])) { asGEDCOM = true; continue; }
        fhmwg::ImageMetadata md;
        try {
            md.parseFile(argv[i]);
        } catch (XMP_Error ex) {
            fprintf(stderr, "CRASHED with error %d:\n  %s\n", ex.GetID(), ex.GetErrMsg());
            throw ex;
        }
        if (asGEDCOM) md.dumpGEDCOM(stdout);
        else { md.dumpJSON(stdout); putc('\n', stdout); }
    }
		
	SXMPFiles::Terminate();
	SXMPMeta::Terminate();

    return 0;
}

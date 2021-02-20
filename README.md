# How to build

1. Get the Adobe XMP Toolkit SDK <https://github.com/adobe/XMP-Toolkit-SDK/>
2. Build Adobe XMP  Toolkit SDK
    - See the README in the `build` directory for your OS
    - This involves downloading <https://www.zlib.net/> and <https://sourceforge.net/projects/expat/>. It was not clear to me, but for clarity the versioned folders inside those archives are not to be perserved; i.e., you should end up with `third-party/expat/lib/` not `third-party/expat/expat-2.2.10/lib/`
    - I had to make a few other changes on my system to reflect that I was not set up for highly secure builds:
        - My `gcc` was built with `--disable-libssp` (see `gcc -v` to check yours), so in `build/ProductConfig.cmake` I removed `${XMP_GCC_LIBPATH}/libssp.a` from the `XMP_PLATFORM_LINK` definition
        - CMake misidentified my secure random number library. I thought about fixing its detection logic, but as I did not need security for my tool I instead added `#define XML_POOR_ENTROPY`{.c} to `third-party/expat/lib/xmlparse.c`
3. Get the latest `json.hpp` from <https://github.com/nlohmann/json/releases> and place it in the same directory as the `hpp` files from this project
4. Adjust `Makefile` in this project
    - I've hard-coded paths from my Linux machine. You'll probably need to change `XMP_BASE` and `XMP_LIB`, and if you are not on Linux may need to change a lot more. The Adobe XMP SDK has a directory `samples` that uses `cmake` to make cross-platform builds, which might be useful if you find my Makefile problematic
    - I've pinned the Makefile to static linking, which simplifies things somewhat as it avoids the need for threading.

# Motivation and design notes

The primary purpose of this project is to provide a public-domain code base others may use, modify, or refer to freely in their own implementations of the FHWMG recommendations.

I chose to use C++ and the Adobe XMP Toolkit based on a review of XMP processing libraries. I noted the toolkit seemed to be the root of many other libraries:

- C++ Adobe XMP Toolkit is wrapped by
    - C++ [exiv2](https://www.exiv2.org/), which is wrapped by
        - GObject [gexiv2](https://wiki.gnome.org/Projects/gexiv2), which is wrapped by
            - Rust [rexvi2](https://github.com/felixc/rexiv2)
    - C [exempi](https://libopenraw.freedesktop.org/exempi/), which is wrapped by
        - Python [XMP toolkit](https://pypi.org/project/python-xmp-toolkit/)
    - NodeJS [xmp-toolkit](https://github.com/Lambda-IT/xmp-toolkit)
- Go [go-xmp](https://github.com/trimmer-io/go-xmp) is a separate implementation
- Perl [exiftool](https://exiftool.org/) is a separate implementation

Several EXIF libraries (e.g. [node-exif](https://github.com/tj/node-exif), [libexif](https://libexif.github.io/), [exif.js](https://github.com/exif-js/exif-js)) also have some XMP support, some simply returning its raw XML/RDF string while others parse it to some degree. I have not yet looked into any of those in more detail.

# `parser`

The example `parser` program accepts one or more image file from the command line, parses their metadata, and prints a representation of the FHMWG-recommended subset of that metadata to the command line. By default, the output is in JSON format, one line per image file. If given the `-g` flag, a GEDCOM-like representation s used instead.

For example, to extract the FHMWG-compatible data from the example image shown at <https://www.iptc.org/std/photometadata/examples/image-region-examples/>, you'd download the [4 Heads](https://www.iptc.org/std/photometadata/examples/image-region-examples/images/photo-4iptc-heads.jpg) resource and run

```bash
make
./parser photo-4iptc-heads.jpg
```

The parser is fairly forgiving, reading other dates if there is no date, people not in a region, and other suggested XMP data from the specification.

Additional features to add:

- [ ] Extract image dimensions and convert pixel-coordinate regions to relative regions
- [ ] Use EXIF and IPTC IIM backups when no XMP field is available
- [ ] Add code documentation
- [ ] Create daemon-mode with sockets for parsing as a service
- [ ] Add support for pre-IPTC regions:
    - [ ] the Microsoft People region (see [spec](https://docs.microsoft.com/en-us/windows/win32/wic/-wic-people-tagging?redirectedfrom=MSDN); this is always a relative rectangle and always stores a single person name
    - [ ] Metadata Working Group region (see [archive of spec](https://web.archive.org/web/20180919181934/www.metadataworkinggroup.org/pdf/mwg_guidance.pdf) page 53; this is much like IPTC regions in design, with the same 3 area types and relative coordinates. However, it does not have nested strutures and cannot distinguish between people and other tagged items of interest


## JSON example output

AltLangs are given as a JSON-LD compatible language map.

```json
{"title":{"en":"Boutros Ghali","cop":"Ⲡⲉⲧⲣⲟⲥ Ⲅⲁⲗⲓ"}
,"caption":{"en":"Boutros Ghali at Naela Chohan's art exhibition for the International Women's Day at UNESCO"}
,"event":{"x-default":"Naela Chohan's art exhibition"}
,"date":"2002-03-05"
,"albums":
  [{"name":"International Women's Day"
   ,"id":"https://example.com/album/iwd"
   }
  ,{"name":"Pictures of UN officials"}
  ]
,"locations":
  [{"name":{"x-default":"UNESCO, Paris, France"}
   ,"latitude":48.8495999
   ,"longitude":2.30588425
   ,"ids":
     ["https://catalogue.bnf.fr/ark:/12148/cb13742945j"
     ,"https://d-nb.info/gnd/2152375-7"
     ]
   }
  ]
,"people":
  [{"name":{"cop-i-default":"Ⲡⲉⲧⲣⲟⲥ Ⲡⲉⲧⲣⲟⲥ-Ⲅⲁⲗⲓ","en":"Boutros Boutros-Ghali"}
   ,"description":{"en":"Sixth secretary-general of the UN"}
   ,"ids":["https://www.worldcat.org/identities/lccn-n82164415"]
   ,"circle":{"x":0.5,"y":0.3,"rx":0.3}
   }
  ,{"name":{"x-default":"unknown photographer"}}
  ]
,"objects":
  [{"title":{"en":"Painting of rolling fields"}
   ,"rectangle":{"x":0.7,"y":0,"w":0.3,"h":0.5}
   }
  ,{"title":{"fr":"Placard décrivant la peinture"}
   ,"polygon":
     [{"x":0.3,"y":0.5}
     ,{"x":0.3,"y":0.6}
     ,{"x":0.35,"y":0.6}
     ,{"x":0.4,"y":0.5}
     ]
   }
  ]
}
```


## GEDCOM schema

AltLangs are given with a payload, a LANG if the default language is not `x-default`, then TRAN + LANG for any non-default languages

IMAGE_METADATA :=
```gedstruct
0 _ALBUM                    {0:M}
  +1 _NAME <Text>           {0:1}
  +1 _ID <IRI>              {0:1}
                            
0 _TITLE <AltLang>          {0:1}
                            
0 _CAPTION <AltLang>        {0:1}
                            
0 _DATE <ISODateTime>       {0:1}
                            
0 _EVENT <AltLang>          {0:1}
    
0 _LOCATION                 {0:1}
  +1 _LATITUDE <Number>     {0:1}
  +1 _LONGITUDE <Number>    {0:1}
  +1 _NAME <AltLang>        {0:1}
  +1 _ID <IRI>              {0:M}

0 _PERSON                   {0:M}
  +1 <<IMAGE_REGION>>       {0:1}
  +1 _NAME <AltLang>        {0:1}
  +1 _DESCRIPTION <AltLang> {0:1}
  +1 _ID <IRI>              {0:M}

0 _OBJECT                   {0:M}
  +1 <<IMAGE_REGION>>       {0:1}
  +1 _TITLE <AltLang>       {0:1}
```

IMAGE_REGION :=
```gedstruct
[
n _CIRCLE                   {1:1}
  +1 _X <Number>            {1:1}
  +1 _Y <Number>            {1:1}
  +1 _RX <Number>           {1:1}
|
n _RECTANGLE                {1:1}
  +1 _X <Number>            {1:1}
  +1 _Y <Number>            {1:1}
  +1 _W <Number>            {1:1}
  +1 _H <Number>            {1:1}
|
n _POLYGON                  {1:1}
  +1 _VERTEX                {3:M}
     +2 _X <Number>         {1:1}
     +2 _Y <Number>         {1:1}
]
```

# `writer`

The writer takes a reference image and an output image name;
it copies the image to the new name and changes the metadata based on JSON provided at the command line.

The JSON format matches that provided by the parser (see [JSON example output]).
If a key is missing, the corresponding metadata is left unaltered (it is not even normalized).
If a key is present, all current metadata that would match that key is removed and the metadata provided in the input (if any) is used instead.

For example, this invocation:

```bash
make
./writer image_i_provide.jpg new_image.jpg <<EOF
{"people":null
,"title":{"x-default":"My Image","ja":"私の写真"}
}
EOF
```

will copy `image_i_provide.jpg` with two changes to the metadata:
any FHMWG-recognized person metadata will be removed
and the title will be set to a string in two languages:
`My Image` as the default and `私の写真` in the Japanese locale.

# Project status

- [x] Implement XMP-to-GEDCOM parser
- [x] Implement XMP-to-JSON parser
- [x] Implement JSON-to-XML writer
- [ ] Implement GEDCOM-to-XML writer
- [x] Verify operation on IPTC example images on a Linux machine
    - <https://iptc.org/std/photometadata/examples/IPTC-PhotometadataRef-Std2017.1.jpg>
    - <https://www.iptc.org/std/photometadata/examples/image-region-examples/images/photo-4iptc-heads.jpg>
- [ ] Perform additional testing
- [ ] Create full documentation
- [ ] Test on multiple operating systems and platforms

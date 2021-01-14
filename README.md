# How to build

1. Get the Adobe XMP SDK <https://github.com/adobe/XMP-Toolkit-SDK/>
2. Build Adobe XMP SDK
    - See the README in the `build` directory for your OS
    - This involves downloading <https://www.zlib.net/> and <https://sourceforge.net/projects/expat/>. It was not clear to me, but for clarity the versioned folders inside those archives are not to be perserved; i.e., you should end up with `third-party/expat/lib/` not `third-party/expat/expat-2.2.10/lib/`
    - I had to make a few other changes on my system to reflect that I was not set up for highly secure builds:
        - My `gcc` was built with `--disable-libssp` (see `gcc -v` to check yours), so in `build/ProductConfig.cmake` I removed `${XMP_GCC_LIBPATH}/libssp.a` from the `XMP_PLATFORM_LINK` definition
        - CMake misidentified my secure random number library. I thought about fixing its detection logic, but as I did not need security for my tool I instead added `#define XML_POOR_ENTROPY`{.c} to `third-party/expat/lib/xmlparse.c`
3. Adjust `Makefile` in this project
    - I've hard-coded paths from my Linux machine. You'll probably need to change `XMP_BASE` and `XMP_LIB`, and if you are not on Linux may need to change a lot more. The Adobe XMP SDK has a directory `samples` that uses `cmake` to make cross-platform builds, which might be useful if you find my Makefile problematic
    - I've pinned the Makefile to static linking, which simplifies things somewhat as it avoids the need for threading.

# parser

The example `parser` program accepted one or more image file from the command line, parses their metadata, and prints a representation of the FHMWG-recommended subset of that metadata to the command line. By default, the output is in JSON format, one line per image file. If given the `-g` flag, a GEDCOM-like representation s used instead.

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
     ["https://catalogue.bnf.fr/ark:/12148/cb152821567"
     ,"https://d-nb.info/gnd/4044660-8"
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
  ,{"title":{"en":"Placard describing painting"}
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

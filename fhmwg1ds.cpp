#include "fhmwg1ds.hpp"
#include <cstring>
#include <cstdlib>
#include <cmath>

namespace fhmwg {

static void jsonString(FILE *f, std::string payload) {
    putc('"', f);
    for(int c : payload) {
        if (c < 0x20) {
            if (c == '\n') fputs("\\n", f);
            else if (c == '\r') fputs("\\r", f);
            else if (c == '\t') fputs("\\t", f);
            else if (c == '\f') fputs("\\f", f);
            else if (c == '\b') fputs("\\b", f);
            else fprintf(f, "\\u%04x", c);
        } else if (c == '"') fputs("\\\"", f);
        else if (c == '\\') fputs("\\\\", f);
        else putc(c, f);
    }
    putc('"', f);
}


static void gedcomBlockText(FILE *f, std::string payload, int level) {
    char *tmp = strdup(payload.c_str());
    bool init = 1;
    for(;;) {
        char *delim = strpbrk(tmp, "\n\r");
        if (!delim) {
            fprintf(f, "%s\n", tmp);
            break;
        } else {
            if (delim[0] == '\r' && delim[1] == '\n') {
                delim[0] = delim[1] = 0;
                delim += 2;
            } else {
                delim[0] = 0;
                delim += 2;
            }
            fprintf(f, "%s\n", tmp);
            tmp = delim;
            if (!init) fprintf(f, "%d CONT ", level+1);
        }
        tmp = delim;
    }
    free(tmp);
}

void AltLang::dumpGEDCOM(FILE *f, int level) {
    gedcomBlockText(f, this->entries[0].text, level);
    if (this->entries[0].lang != "x-default")
        fprintf(f, "%d LANG %s\n", level+1, this->entries[0].lang.c_str());
    for(size_t i=1; i<this->entries.size(); i+=1) {
        fprintf(f, "%d TRAN ", level+1);
        gedcomBlockText(f, this->entries[i].text, level+1);
        fprintf(f, "%d LANG %s\n", level+2, this->entries[i].lang.c_str());
    }
}
void AltLang::dumpJSON(FILE *f) {
    char pfx = '{';
    for(LangStr x : this->entries) {
        fputc(pfx, f);
        pfx = ',';
        jsonString(f, x.lang);
        fputc(':', f);
        jsonString(f, x.text);
    }
    if (pfx == '{') fputc('{', f);
    fputc('}', f);
}



void Album::dumpGEDCOM(FILE *f) {
    fprintf(f, "0 _ALBUM\n");
    if (this->name.size() > 0) {
        fprintf(f, "1 _NAME ");
        gedcomBlockText(f, this->name, 1);
    }
    if (this->id.size() > 0) {
        fprintf(f, "1 _ID %s\n", this->id.c_str());
    }
}
void Album::dumpJSON(FILE *f) {
    char pfx = '{';
    if (this->name.size() > 0) {
        putc(pfx, f);
        fputs("\"name\":", f);
        jsonString(f, this->name);
        pfx = ',';
    }
    if (this->id.size() > 0) {
        putc(pfx, f);
        fputs("\"id\":", f);
        jsonString(f, this->id);
    }
    if (pfx != '{') putc('}', f);
}

void Location::dumpGEDCOM(FILE *f) {
    fprintf(f, "0 _LOCATION\n");
    if (!std::isnan(this->lat) && !std::isnan(this->lon)) {
        fprintf(f, "1 _LATITUDE %.15f\n1 _LONGITUDE %.15f\n", this->lat, this->lon);
    }
    if (this->name.entries.size() > 0) {
        fprintf(f, "1 _NAME ");
        this->name.dumpGEDCOM(f, 1);
    }
    for(IRI id : this->ids) {
        fprintf(f, "1 _ID %s\n", id.c_str());
    }
}
void Location::dumpJSON(FILE *f) {
    char pfx = '{';
    if (this->name.entries.size() > 0) {
        putc(pfx, f); pfx=',';
        fputs("\"name\":", f);
        this->name.dumpJSON(f);
    }
    if (!std::isnan(this->lat) && !std::isnan(this->lon)) {
        putc(pfx, f); pfx=',';
        fprintf(f, "\"latitude\":%.15g,\"longitude\":%15g", this->lat, this->lon);
    }
    if (this->ids.size() > 0) {
        putc(pfx, f); pfx='[';
        fputs("\"ids\":", f);
        for(IRI id : this->ids) {
            putc(pfx, f); pfx=',';
            jsonString(f, id);
        }
        putc(']', f);
    }
    if (pfx != '{') putc('}', f);
}


void Region::dumpGEDCOM(FILE *f, int level) {
    switch(this->type) {
        case Region::Types::NONE: break;
        case Region::Types::CIRCLE:
            fprintf(f, "%d _CIRCLE\n", level);
            fprintf(f, "%d _X %.15f\n", level+1, this->circ.x);
            fprintf(f, "%d _Y %.15f\n", level+1, this->circ.y);
            fprintf(f, "%d _RX %.15f\n", level+1, this->circ.rx);
        break;
        case Region::Types::POLYGON:
            fprintf(f, "%d _POLYGON\n", level);
            for(std::pair<double,double> pt : this->pts) {
                fprintf(f, "%d _VERTEX\n", level+1);
                fprintf(f, "%d _X %.15f\n", level+2, std::get<0>(pt));
                fprintf(f, "%d _Y %.15f\n", level+2, std::get<1>(pt));
            }
        break;
        case Region::Types::RECTANGLE:
            fprintf(f, "%d _RECTANGLE\n", level);
            fprintf(f, "%d _X %.15f\n", level+1, this->rect.x);
            fprintf(f, "%d _Y %.15f\n", level+1, this->rect.y);
            fprintf(f, "%d _W %.15f\n", level+1, this->rect.w);
            fprintf(f, "%d _H %.15f\n", level+1, this->rect.h);
        break;
    }
}
bool Region::dumpJSON(FILE *f, char pfx) {
    switch(this->type) {
        case Region::Types::NONE: return false;
        case Region::Types::CIRCLE:
            fprintf(f, "%c\"circle\":{\"x\":%.15g,\"y\":%.15g,\"rx\":%.15g}", 
            pfx, this->circ.x, this->circ.y, this->circ.rx);
            return true;
        case Region::Types::POLYGON:
            fprintf(f, "%c\"polygon\":", pfx);
            pfx = '[';
            for(std::pair<double,double> pt : this->pts) {
                fprintf(f, "%c{\"x\":%.15g,\"y\":%.15g}", pfx, std::get<0>(pt), std::get<1>(pt));
                pfx = ',';
            }
            if (pfx == '[') fputs("[]", f);
            else putc(']',f);
            return true;
        case Region::Types::RECTANGLE:
            fprintf(f, "%c\"rectangle\":{\"x\":%.15g,\"y\":%.15g,\"w\":%.15g,\"h\":%.15g}", 
            pfx, this->rect.x, this->rect.y, this->rect.w, this->rect.h);
            return true;
    }
    return false;
}

void Person::dumpGEDCOM(FILE *f) {
    fprintf(f, "0 _PERSON\n");
    if (this->region.type) this->region.dumpGEDCOM(f, 1);
    if (this->name.entries.size() > 0) {
        fprintf(f, "1 _NAME ");
        this->name.dumpGEDCOM(f, 1);
    }
    if (this->description.entries.size() > 0) {
        fprintf(f, "1 _DESCRIPTION ");
        this->description.dumpGEDCOM(f, 1);
    }
    for(IRI id : this->ids) {
        fprintf(f, "1 _ID %s\n", id.c_str());
    }
}
void Person::dumpJSON(FILE *f) {
    char pfx = '{';
    if (this->name.entries.size() > 0) {
        putc(pfx, f); pfx=',';
        fputs("\"name\":", f);
        this->name.dumpJSON(f);
    }
    if (this->description.entries.size() > 0) {
        putc(pfx, f); pfx=',';
        fputs("\"description\":", f);
        this->description.dumpJSON(f);
    }
    if (this->ids.size() > 0) {
        putc(pfx, f); pfx='[';
        fputs("\"ids\":", f);
        for(IRI id : this->ids) {
            putc(pfx, f); pfx=',';
            jsonString(f, id);
        }
        putc(']', f);
    }
    if (this->region.dumpJSON(f, pfx)) pfx = ',';
    if (pfx != '{') putc('}', f);
}


void Object::dumpGEDCOM(FILE *f) {
    fprintf(f, "0 _OBJECT\n");
    if (this->region.type) this->region.dumpGEDCOM(f, 1);
    if (this->title.entries.size() > 0) {
        fprintf(f, "1 _TITLE ");
        this->title.dumpGEDCOM(f, 1);
    }
}
void Object::dumpJSON(FILE *f) {
    char pfx = '{';
    if (this->title.entries.size() > 0) {
        putc(pfx, f); pfx=',';
        fputs("\"title\":", f);
        this->title.dumpJSON(f);
    }
    if (this->region.dumpJSON(f, pfx)) pfx = ',';
    if (pfx != '{') putc('}', f);
}

void ImageMetadata::dumpGEDCOM(FILE *f) {
    if (title.entries.size() > 0) {
        fprintf(f, "0 _TITLE ");
        title.dumpGEDCOM(f, 1);
    }
    if (caption.entries.size() > 0) {
        fprintf(f, "0 _CAPTION ");
        caption.dumpGEDCOM(f, 1);
    }
    if (event.entries.size() > 0) {
        fprintf(f, "0 _EVENT ");
        title.dumpGEDCOM(f, 1);
    }
    if (date.size() > 0)
        fprintf(f, "0 _DATE %s\n", date.c_str());
    
    for(Album x : albums) x.dumpGEDCOM(f);
    for(Location x : locations) x.dumpGEDCOM(f);
    for(Person x : people) x.dumpGEDCOM(f);
    for(Object x : objects) x.dumpGEDCOM(f);
}

void ImageMetadata::dumpJSON(FILE *f, bool newlines) {
    char pfx = '{';
    if (title.entries.size() > 0) {
        putc(pfx, f); pfx = ',';
        fputs("\"title\":", f);
        title.dumpJSON(f);
        if(newlines) putc('\n', f);
    }
    if (caption.entries.size() > 0) {
        putc(pfx, f); pfx = ',';
        fputs("\"caption\":", f);
        caption.dumpJSON(f);
        if(newlines) putc('\n', f);
    }
    if (event.entries.size() > 0) {
        putc(pfx, f); pfx = ',';
        fputs("\"event\":", f);
        event.dumpJSON(f);
        if(newlines) putc('\n', f);
    }
    if (date.size() > 0) {
        putc(pfx, f); pfx = ',';
        fputs("\"date\":", f);
        jsonString(f, date);
        if(newlines) putc('\n', f);
    }
    
    if (albums.size() > 0) {
        putc(pfx, f); pfx = '[';
        fputs("\"albums\":", f);
        for(Album x : albums) {
            putc(pfx, f); pfx=',';
            x.dumpJSON(f);
            if(newlines) fputs("\n  ", f);
        }
        putc(']',f);
        if(newlines) putc('\n', f);
    }

    if (locations.size() > 0) {
        putc(pfx, f); pfx = '[';
        fputs("\"locations\":", f);
        for(Location x : locations) {
            putc(pfx, f); pfx=',';
            x.dumpJSON(f);
            if(newlines) fputs("\n  ", f);
        }
        putc(']',f);
        if(newlines) putc('\n', f);
    }

    if (people.size() > 0) {
        putc(pfx, f); pfx = '[';
        fputs("\"people\":", f);
        for(Person x : people) {
            putc(pfx, f); pfx=',';
            x.dumpJSON(f);
            if(newlines) fputs("\n  ", f);
        }
        putc(']',f);
        if(newlines) putc('\n', f);
    }
 
    if (objects.size() > 0) {
        putc(pfx, f); pfx = '[';
        fputs("\"objects\":", f);
        for(Object x : objects) {
            putc(pfx, f); pfx=',';
            x.dumpJSON(f);
            if(newlines) fputs("\n  ", f);
        }
        putc(']',f);
        if(newlines) putc('\n', f);
    }
    
    if (pfx != '{') putc('}', f);
}


} // namespace fhmwg


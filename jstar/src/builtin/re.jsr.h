// WARNING: this is a file generated automatically by the build process. Do not modify.
const char *re_jsr =
"class RegexException is Exception end\n"
"native match(str, regex, off=0)\n"
"native find(str, regex, off=0)\n"
"native gsub(str, regex, sub, num=0)\n"
"native gmatch(str, regex)\n"
"fun igmatch(str, regex)\n"
"    return _IGMatch(str, regex)\n"
"end\n"
"class _IGMatch\n"
"    fun new(s, r)\n"
"        this.s, this.r = s, r\n"
"        this.off, this.endl = 0, null\n"
"    end\n"
"    fun __iter__(_)\n"
"        var res = find(this.s, this.r, this.off)\n"
"        if !res then return null end\n"
"        \n"
"        var b, e = res\n"
"        while e == this.endl and e - b == 0 do\n"
"            res = find(this.s, this.r, this.off += 1)\n"
"            if res then return null end\n"
"            b, e = res\n"
"        end\n"
"        var resLen = #res\n"
"        this.off = this.endl = e\n"
"        if   resLen == 2 then\n"
"            return this.s.slice(b, e)\n"
"        elif resLen == 3 then\n"
"            return res[2]\n"
"        else\n"
"            return res.slice(2, resLen)\n"
"        end\n"
"    end\n"
"    fun __next__(match)\n"
"        return match\n"
"    end\n"
"end\n"
;
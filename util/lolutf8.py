import csv
import re
lolcsv='LandonlineCharSet.csv'

mapping={
    'NO-BREAK SPACE': '\\s',
    'BROKEN BAR': '|',
    'SOFT HYPHEN': 'delete',
    'MIDDLE DOT': '.',
    'RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK': '"',
    'LEFT-POINTING DOUBLE ANGLE QUOTATION MARK': '"',
    'ACUTE ACCENT': "'",
    }

cfgheader="""
# bde_copy configuration for Landonline UTF8 characters
#
# Maps allowed UTF8 characters in Landonline text strings.
# Unmapped UTF8 characters are replaced with ? and generate a warning

utf8_encoding enforced
utf8_replace_invalid delete "Invalid UTF8 character"
utf8_replace_unmapped ? "Unmapped UTF8 character"

# Character mapping for text fields.  Each replace consists of a 
# character to be replaced, and a list of characters to replace it
# A missing list or the word "none" can be used to indicate nothing is
# output.

"""

uchars=[]
with open(lolcsv) as csvf:
    csvr=csv.reader(csvf)
    # Skip two header lines
    header=csvr.next()
    header=csvr.next()

    for row in csvr:
        if len(row) < 9:
            continue
        ucodept=row[4]
        if not re.match(r"^U\+[0-9A-F]{4}$",ucodept):
            continue
        uname=row[7]
        uval=int(ucodept[2:],16)
        # ASCII values are not modified..
        if uval < 128:
            continue
        uchars.append({'code':uval,'ucode':ucodept,'name':uname})

with open('bde_copy.cfg.lolutf8','w') as cfgf:
    cfgf.write(cfgheader)
    for char in uchars:
        ucode=char['ucode']
        name=char['name']
        trans='?'
        match=re.match(r'^LATIN (SMALL|CAPITAL) LETTER ([A-Z]) ',name)
        if name in mapping:
            trans=mapping[name]
        elif match:
            trans=match.group(2)
            if match.group(1) == 'SMALL':
                trans=trans.lower()
        elif '<control>' in name:
            trans='delete'

        cfgf.write("\n# Code point {0} {1}\nreplace \\u{2} {3}\n".format(
            ucode,name,ucode[2:],trans))




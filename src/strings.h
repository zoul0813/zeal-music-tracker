#ifndef STRINGS_H
#define STRINGS_H

#define TEXT_HIGHLIGHT(character) "\x1B\x74" character

#define CH_CARET_LEFT  "\x11"
#define CH_CARET_RIGHT "\x10"
#define CH_CARET_UP    "\x1E"
#define CH_CARET_DOWN  "\x1F"
#define CH_ARROW_RIGHT "\x1A"
#define CH_BULLET      "\xF9"

extern const char STR_DIALOG_CONFIRM[];
extern const char STR_ENTER_TO_CLOSE[];

#endif

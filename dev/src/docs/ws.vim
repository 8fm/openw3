" Vim syntax file
" Language:	WitcherScript
" Maintainer:	Kostas Michalopoulos
" Last Change:	2013 May 28

if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

" Keywords
syn keyword wsPrimary class var extends in array return if for state switch case default break continue do while else enum struct super parent virtual_parent new delete function event savepoint local final const editable timer cleanup inlined latent abstract transient out optional import private protected public exec entry storyscene saved quest reward
syn keyword wsType byte int float bool name string true false void NULL this

" Identifier
syn match wsIdentifier display "\(\a\|_\)\(\w\|_\)*"

" Type specialization
syn match wsSpecialization display "<\s*\w*\s*>"

" Function call
syn match wsCall display "\(\a\|_\)\(\w\|_\)*\s*(" contains=wsSpecialChar

" Strings
syn region wsStringSQ start=+L\='+ end=+'+
syn region wsStringDQ start=+L\="+ end=+"+

" Numbers
syn case ignore
syn match wsNumber display "[^\w\|_]\d\+\(u\=l\{0,2}\|ll\=u\)\>"
syn match wsNumber display "[^\w\|_]0x\x\+\(u\=l\{0,2}\|ll\=u\)\>"
syn match wsFloat display "[^\w\|_]\d\+f"
syn match wsFloat display "[^\w\|_]\d\+\.\d*\(e[-+]\=\d\+\)\=[fl]\="
syn match wsFloat display "[^\w\|_]\.\d\+\(e[-+]\=\d\+\)\=[fl]\=\>"
syn match wsFloat display "[^\w\|_]\d\+e[-+]\=\d\+[fl]\=\>"
syn match wsOctal display "[^\w\|_]0\o\+\(u\=l\{0,2}\|ll\=u\)\>" contains=wsOctalZero
syn match wsOctalZero display contained "[^\w\|_]\<0"
syn case match

" Special characters
syn match wsSpecialChar display "{\|}\|(\|)\|;\|:\|,"

" Operators
syn match wsOperator display "\.\|=\|+\|-\|*\|/\|%\|&\|&&\||\|||\|^"

" Comments
syn region wsCommentLine start="//" end="$" keepend
syn region wsCommentMulti start="/\*" end="\*/" fold extend

" Type cast
syn match wsCast display "\(\w\s*\)\@<!(\s*\(\a\|_\)\(\w\|_\)*\s*)"

" Default highlighting
if version >= 508 || !exists("did_ws_syntax_inits")
  if version < 508
    let did_ws_syntax_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif
  HiLink wsPrimary              Statement
  HiLink wsType                 Type
  HiLink wsSpecialization       Special
  HiLink wsStringSQ             String
  HiLink wsStringDQ             String
  HiLink wsNumber               Number
  HiLink wsFloat                Number
  HiLink wsOctal                Number
  HiLink wsCommentLine          Comment
  HiLink wsCommentMulti         Comment
  HiLink wsSpecialChar          SpecialChar
  HiLink wsOperator             Operator
  HiLink wsCall                 Macro
  HiLink wsCast                 Special
  delcommand HiLink
endif

let b:current_syntax = "ws"

" vim: ts=8


#!/bin/sh

# ping pong

./cos_loader \
"c0.o, ;llboot.o, ;*fprr.o, ;mm.o, ;print.o, ;boot.o, ;\
\
!mpool.o,a3;!trans.o,a6;!l.o,a1;!te.o,a3;!e.o,a4;!stat.o,a25;!cbuf.o,a5;!, a6;(!bot.o=micbuf3.o), ;(!mid.o=micbuf2.o), ;(!top.o=micbuf1.o),a9;!vm.o,a1;!va.o,a2:\
\
c0.o-llboot.o;\
fprr.o-print.o|[parent_]mm.o|[faulthndlr_]llboot.o;\
mm.o-[parent_]llboot.o|print.o;\
boot.o-print.o|fprr.o|mm.o|llboot.o;\
l.o-fprr.o|mm.o|print.o;\
te.o-cbuf.o|print.o|fprr.o|mm.o|va.o;\
e.o-cbuf.o|fprr.o|print.o|mm.o|l.o|va.o;\
stat.o-cbuf.o|te.o|fprr.o|l.o|print.o|e.o;\
\
cbuf.o-boot.o|fprr.o|print.o|l.o|mm.o|va.o|mpool.o;\
\
mpool.o-print.o|fprr.o|mm.o|boot.o|va.o|l.o;\
\
vm.o-fprr.o|print.o|mm.o|l.o|boot.o;\
va.o-fprr.o|print.o|mm.o|l.o|boot.o|vm.o;\
\
trans.o-fprr.o|l.o|cbuf.o||mm.o|va.o|e.o|print.o;\
\
top.o-cbuf.o|fprr.o|mid.o|print.o;\
mid.o-fprr.o|print.o|bot.o;\
bot.o-fprr.o|print.o|cbuf.o||va.o|l.o|mm.o|cbuf.o\
" ./gen_client_stub

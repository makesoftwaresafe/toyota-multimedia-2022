options(digits=4)
loop <- scan(file1, list(day=0, sec=0, offset=0, freq=0, tc=0))
loop$offset <- loop$offset * 1e6
str <- paste("eps/", file1, ".eps", sep="")
postscript(str,  ,  ,  , 5, pointsize=18)
par(mgp=c(1, 0, 0), tck=0.03, mar=c(2, 2, 1, 1))
plot(loop$sec, loop$offset, type="l", xlab=paste("MJD", loop$day, "Time (s)"), ylab="PLL Offset (us)", ylim=c(-400, 400))

#ifdef __QNXNTO__
#ifdef __USESRCVERSION
.section .ident, "SM",%progbits,1;
.asciz "$URL: http://svn.ott.qnx.com/product/tags/7.0.0/GA/lib/io-pkt/dist/ntp/scripts/stats/loop.S $ $Rev: 806186 $";
.previous
#endif
#endif

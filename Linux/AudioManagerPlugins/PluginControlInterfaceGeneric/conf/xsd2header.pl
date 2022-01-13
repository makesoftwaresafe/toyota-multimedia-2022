#!/usr/bin/perl -w
use XML::Parser;
use File::Basename;
use IO::File;
use Scalar::Util qw(looks_like_number);

$simpletypename="";
$enumerationvalue="0";
$enumerationstring="";
$documentation="";
$num_args = $#ARGV + 1;
my $xmlfile = shift;
if ($num_args != 1 || ($xmlfile eq "--help") || ($xmlfile eq "-h") || ($xmlfile eq "\/?"))
{
	print "------------------------------------------------------------\n";
    print "Usage: xsd2header.pl <filename.xsd>\n";
	print "The output is printed on stdout, redirect to desired file. \n";	
	print "example: xsd2header.pl customtypes.xsd > customtypes.h     \n";	
	print "------------------------------------------------------------\n";
    exit;
}

die "Cannot find file \"$xmlfile\""
       unless -f $xmlfile;
my $parser = new XML::Parser;

$outputfilename = basename($xmlfile);
$outputfilename =~ s{\.[^.]+$}{};
$parser->setHandlers(      Start => \&startElement,
                                         End => \&endElement,
                                         Char => \&characterData,
                                         Default => \&default);

print "\/* This is a generated file, Please don't modify directly                     *\/\n";
print "\/* Generation Timestamp " . localtime() . "                              *\/\n";
print "\n";
print "\#ifndef " . "_" . uc $outputfilename . "_H_\n";
print "\#define " . "_" . uc $outputfilename . "_H_\n";
print "\n";
print "#include \"audiomanagertypes.h\"\n\n";
print "using namespace am;";
print "\n";
$parser->parsefile($xmlfile);

print "\n";
print "\#endif \/* " . "_" . uc $outputfilename . "_H_ *\/\n";

# This call back get called when start of a tag is encountered
sub startElement
{
	my $arg1 = shift;
	my $tagname = shift;
	my $attributename = shift;
	my $attributevalue = shift;
	if($tagname eq "xsd:simpleType")
	{
		if($attributename eq "name")
		{
			$simpletypename = $attributevalue;
		    if($simpletypename eq "am_SinkID_e")
			{
				$simpletypename = "am_sinkID_t";
			}
		    if($simpletypename eq "am_SourceID_e")
			{
				$simpletypename = "am_sourceID_t";
			}
			if($simpletypename eq "am_ClassID_e")
			{
				$simpletypename = "uint16_t";
			}
		}
		print "\n";
		print "\/* defines for the " . $simpletypename .  " *\/\n";
	}
	if($tagname eq "xsd:enumeration")
	{
		if($attributename eq "value")
		{
			$enumerationstring = $attributevalue;
		}
	}
	if($tagname eq "xsd:documentation")
	{
		if(($simpletypename ne "") && ($enumerationstring ne "") )
		{
			$documentation = "parse";
		}
	}
}

# This call back get called when end of a tag is encountered
sub endElement
{
	my $arg1 = shift;
	my $tagname = shift;
	if($tagname eq "xsd:simpleType")
	{
		$simpletypename = "";
	    $enumerationvalue="0";
	}
	if($tagname eq "xsd:enumeration")
	{
		$enumerationstring = "";
	}
	if($tagname eq "xsd:documentation")
	{
		$documentation = "";
	}

}

# This call back get called when text between the tags is encountered
sub characterData
{
	my $number = "0";
	my $arg1 = shift;
	my $insidetag = shift;
	if($documentation eq "parse")
	{

		$first_word = (split(/\s+/, $insidetag))[0];
		if(looks_like_number($first_word))
		{
			$enumerationvalue = $first_word;
		}
		$documentation="";
		print "A_CONST " . $simpletypename . " " . $enumerationstring . " = ".  $enumerationvalue . ";\n";
		$enumerationvalue = $enumerationvalue + 1 ;
	}
}

sub default
{
}

--TEST--
Core dump when returning FALSE in a handler
--SKIPIF--
<?php include("skipif.inc"); ?>
--INI--
magic_quotes_runtime=0
--FILE--
<?php
function handle_files_all($xh, $proto, $rest_uri)
{
	return FALSE;
}

$xh = xslt_create();
xslt_setopt($xh, XSLT_SABOPT_FILES_TO_HANDLER);
$xmlstring='<?xml version="1.0" encoding="ISO-8859-1"?>
<!DOCTYPE qa SYSTEM "file://qa.dtd">
<qa>
	<test type="simple">PHP QA</test>
</qa>';
$xslstring='<?xml version="1.0" encoding="ISO-8859-1"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
	<xsl:output method="text" omit-xml-declaration="yes" encoding="ISO-8859-1" />
	<xsl:param name="insertion">Test failed</xsl:param>
	<xsl:template match="/qa">
		<xsl:apply-templates select="test" />
	</xsl:template>
	<xsl:template match="test">
		<xsl:value-of select="concat(@type, \': \', .)" />
	</xsl:template>
</xsl:stylesheet>';
xslt_set_scheme_handlers($xh, array('get_all' => 'handle_files_all'));
$result = xslt_process($xh, 'arg:/_xml', 'arg:/_xsl', NULL, array('/_xml' => $xmlstring, '/_xsl' => $xslstring));
echo $result;
xslt_free($xh);
?>
--EXPECT--
simple: PHP QA

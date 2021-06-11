--TEST--
Test output_add_rewrite_var() function basic feature
--EXTENSIONS--
session
--INI--
session.trans_sid_tags="a=href,area=href,frame=src,form="
url_rewriter.tags="a=href,area=href,frame=src,form="
--FILE--
<?php
    ob_start();
// Common setting
ini_set('url_rewriter.hosts', 'php.net,www.php.net');
ini_set('session.trans_sid_hosts', 'php.net,www.php.net');
ini_set('session.use_only_cookies', 0);
ini_set('session.use_cookies', 0);
ini_set('session.use_strict_mode', 0);
session_id('testid');

output_add_rewrite_var('<name>', '<value>');
?>
Without session
<a href=""> </a>
<a href="./foo.php"> </a>
<a href="//php.net/foo.php"> </a>
<a href="http://php.net/foo.php"> </a>
<a href="bad://php.net/foo.php"> </a>
<a href="//www.php.net/foo.php"> </a>

<form method="get"> </form>
<form action="./foo.php" method="get">
<form action="//php.net/bar.php" method="get">
<form action="http://php.net/bar.php" method="get">
<form action="bad://php.net/bar.php" method="get">
<form action="//www.php.net/bar.php" method="get">

<?php
ini_set('session.use_trans_sid', 0);
session_start();
output_add_rewrite_var('<name>', '<value>');
?>
Test use_trans_sid=0
<a href=""> </a>
<a href="./foo.php"> </a>
<a href="//php.net/foo.php"> </a>
<a href="http://php.net/foo.php"> </a>
<a href="bad://php.net/foo.php"> </a>
<a href="//www.php.net/foo.php"> </a>

<form method="get"> </form>
<form action="./foo.php" method="get"> </form>
<form action="//php.net/bar.php" method="get"> </form>
<form action="http://php.net/bar.php" method="get"> </form>
<form action="bad://php.net/bar.php" method="get"> </form>
<form action="//www.php.net/bar.php" method="get"> </form>

<?php
session_commit();
ini_set('session.use_trans_sid', 1);
output_reset_rewrite_vars();
session_start();
output_add_rewrite_var('<NAME>', '<VALUE>');
?>
Test use_trans_sid=1
<a href=""> </a>
<a href="./foo.php"> </a>
<a href="//php.net/foo.php"> </a>
<a href="http://php.net/foo.php"> </a>
<a href="bad://php.net/foo.php"> </a>
<a href="//www.php.net/foo.php"> </a>

<form method="get"> </form>
<form action="./foo.php" method="get"> </form>
<form action="//php.net/bar.php" method="get"> </form>
<form action="http://php.net/bar.php" method="get"> </form>
<form action="bad://php.net/bar.php" method="get"> </form>
<form action="//www.php.net/bar.php" method="get"> </form>
--EXPECT--
Without session
<a href="?%3CNAME%3E=%3CVALUE%3E"> </a>
<a href="./foo.php?%3CNAME%3E=%3CVALUE%3E"> </a>
<a href="//php.net/foo.php?%3CNAME%3E=%3CVALUE%3E"> </a>
<a href="http://php.net/foo.php?%3CNAME%3E=%3CVALUE%3E"> </a>
<a href="bad://php.net/foo.php"> </a>
<a href="//www.php.net/foo.php?%3CNAME%3E=%3CVALUE%3E"> </a>

<form method="get"><input type="hidden" name="&lt;NAME&gt;" value="&lt;VALUE&gt;" /> </form>
<form action="./foo.php" method="get"><input type="hidden" name="&lt;NAME&gt;" value="&lt;VALUE&gt;" />
<form action="//php.net/bar.php" method="get"><input type="hidden" name="&lt;NAME&gt;" value="&lt;VALUE&gt;" />
<form action="http://php.net/bar.php" method="get"><input type="hidden" name="&lt;NAME&gt;" value="&lt;VALUE&gt;" />
<form action="bad://php.net/bar.php" method="get">
<form action="//www.php.net/bar.php" method="get"><input type="hidden" name="&lt;NAME&gt;" value="&lt;VALUE&gt;" />

Test use_trans_sid=0
<a href="?%3CNAME%3E=%3CVALUE%3E"> </a>
<a href="./foo.php?%3CNAME%3E=%3CVALUE%3E"> </a>
<a href="//php.net/foo.php?%3CNAME%3E=%3CVALUE%3E"> </a>
<a href="http://php.net/foo.php?%3CNAME%3E=%3CVALUE%3E"> </a>
<a href="bad://php.net/foo.php"> </a>
<a href="//www.php.net/foo.php?%3CNAME%3E=%3CVALUE%3E"> </a>

<form method="get"><input type="hidden" name="&lt;NAME&gt;" value="&lt;VALUE&gt;" /> </form>
<form action="./foo.php" method="get"><input type="hidden" name="&lt;NAME&gt;" value="&lt;VALUE&gt;" /> </form>
<form action="//php.net/bar.php" method="get"><input type="hidden" name="&lt;NAME&gt;" value="&lt;VALUE&gt;" /> </form>
<form action="http://php.net/bar.php" method="get"><input type="hidden" name="&lt;NAME&gt;" value="&lt;VALUE&gt;" /> </form>
<form action="bad://php.net/bar.php" method="get"> </form>
<form action="//www.php.net/bar.php" method="get"><input type="hidden" name="&lt;NAME&gt;" value="&lt;VALUE&gt;" /> </form>

Test use_trans_sid=1
<a href="?PHPSESSID=testid&%3CNAME%3E=%3CVALUE%3E"> </a>
<a href="./foo.php?PHPSESSID=testid&%3CNAME%3E=%3CVALUE%3E"> </a>
<a href="//php.net/foo.php?PHPSESSID=testid&%3CNAME%3E=%3CVALUE%3E"> </a>
<a href="http://php.net/foo.php?PHPSESSID=testid&%3CNAME%3E=%3CVALUE%3E"> </a>
<a href="bad://php.net/foo.php"> </a>
<a href="//www.php.net/foo.php?PHPSESSID=testid&%3CNAME%3E=%3CVALUE%3E"> </a>

<form method="get"><input type="hidden" name="&lt;NAME&gt;" value="&lt;VALUE&gt;" /><input type="hidden" name="PHPSESSID" value="testid" /> </form>
<form action="./foo.php" method="get"><input type="hidden" name="&lt;NAME&gt;" value="&lt;VALUE&gt;" /><input type="hidden" name="PHPSESSID" value="testid" /> </form>
<form action="//php.net/bar.php" method="get"><input type="hidden" name="&lt;NAME&gt;" value="&lt;VALUE&gt;" /><input type="hidden" name="PHPSESSID" value="testid" /> </form>
<form action="http://php.net/bar.php" method="get"><input type="hidden" name="&lt;NAME&gt;" value="&lt;VALUE&gt;" /><input type="hidden" name="PHPSESSID" value="testid" /> </form>
<form action="bad://php.net/bar.php" method="get"> </form>
<form action="//www.php.net/bar.php" method="get"><input type="hidden" name="&lt;NAME&gt;" value="&lt;VALUE&gt;" /><input type="hidden" name="PHPSESSID" value="testid" /> </form>

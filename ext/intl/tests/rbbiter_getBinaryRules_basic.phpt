--TEST--
RuleBasedBreakIterator::getBinaryRules(): basic test
--FILE--
<?php
ini_set("intl.error_level", E_WARNING);
ini_set("intl.default_locale", "pt_PT");

$rules = <<<RULES
\$LN = [[:letter:] [:number:]];
\$S = [.;,:];

!!forward;
\$LN+ {1};
\$S+ {42};
!!reverse;
\$LN+ {1};
\$S+ {42};
!!safe_forward;
!!safe_reverse;
RULES;
$rbbi = new RuleBasedBreakIterator($rules);
$rbbi->setText('sdfkjsdf88á.... ,;');;

$br = $rbbi->getBinaryRules();

$rbbi2 = new RuleBasedBreakIterator($br, true);

var_dump($rbbi->getRules(), $rbbi2->getRules());
var_dump($rbbi->getRules() == $rbbi2->getRules());
?>
==DONE==
--EXPECT--
string(128) "$LN = [[:letter:] [:number:]];$S = [.;,:];!!forward;$LN+ {1};$S+ {42};!!reverse;$LN+ {1};$S+ {42};!!safe_forward;!!safe_reverse;"
string(128) "$LN = [[:letter:] [:number:]];$S = [.;,:];!!forward;$LN+ {1};$S+ {42};!!reverse;$LN+ {1};$S+ {42};!!safe_forward;!!safe_reverse;"
bool(true)
==DONE==
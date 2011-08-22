--TEST--
DateTime::diff() days -- spring type3 type2
--CREDITS--
Daniel Convissor <danielc@php.net>
--FILE--
<?php

require 'examine_diff.inc';
define('PHPT_DATETIME_SHOW', PHPT_DATETIME_SHOW_DAYS);
require 'DateTime_data-spring-type3-type2.inc';

?>
--EXPECT--
test_time_spring_type3_prev_type2_prev: DAYS: **30**
test_time_spring_type3_prev_type2_st: DAYS: **0**
test_time_spring_type3_prev_type2_dt: DAYS: **0**
test_time_spring_type3_prev_type2_post: DAYS: **2**
test_time_spring_type3_st_type2_prev: DAYS: **0**
test_time_spring_type3_st_type2_st: DAYS: **0**
test_time_spring_type3_st_type2_dt: DAYS: **0**
test_time_spring_type3_st_type2_post: DAYS: **1**
test_time_spring_type3_dt_type2_prev: DAYS: **0**
test_time_spring_type3_dt_type2_st: DAYS: **0**
test_time_spring_type3_dt_type2_dt: DAYS: **0**
test_time_spring_type3_dt_type2_post: DAYS: **1**
test_time_spring_type3_post_type2_prev: DAYS: **2**
test_time_spring_type3_post_type2_st: DAYS: **1**
test_time_spring_type3_post_type2_dt: DAYS: **1**
test_time_spring_type3_post_type2_post: DAYS: **0**
test_time_spring_type3_stsec_type2_dtsec: DAYS: **0**
test_time_spring_type3_dtsec_type2_stsec: DAYS: **0**

--TEST--
DateTime::sub() -- spring type2 type2
--CREDITS--
Daniel Convissor <danielc@php.net>
--XFAIL--
PHP < 5.4 has bugs
--FILE--
<?php

require 'examine_diff.inc';
define('PHPT_DATETIME_SHOW', PHPT_DATETIME_SHOW_SUB);
require 'DateTime_data-spring-type2-type2.inc';

?>
--EXPECT--
test_time_spring_type2_prev_type2_prev: SUB: 2010-03-13 18:38:28 EST - P+0Y1M2DT16H19M40S = **2010-02-11 02:18:48 EST**
test_time_spring_type2_prev_type2_st: SUB: 2010-03-14 00:10:20 EST - P+0Y0M0DT5H31M52S = **2010-03-13 18:38:28 EST**
test_time_spring_type2_prev_type2_dt: SUB: 2010-03-14 03:16:55 EDT - P+0Y0M0DT7H38M27S = **2010-03-13 18:38:28 EST**
test_time_spring_type2_prev_type2_post: SUB: 2010-03-15 19:59:59 EDT - P+0Y0M2DT1H21M31S = **2010-03-13 18:38:28 EST**
test_time_spring_type2_st_type2_prev: SUB: 2010-03-13 18:38:28 EST - P-0Y0M0DT5H31M52S = **2010-03-14 00:10:20 EST**
test_time_spring_type2_st_type2_st: SUB: 2010-03-14 00:15:35 EST - P+0Y0M0DT0H5M15S = **2010-03-14 00:10:20 EST**
test_time_spring_type2_st_type2_dt: SUB: 2010-03-14 03:16:55 EDT - P+0Y0M0DT2H6M35S = **2010-03-14 00:10:20 EST**
test_time_spring_type2_st_type2_post: SUB: 2010-03-15 19:59:59 EDT - P+0Y0M1DT18H49M39S = **2010-03-14 00:10:20 EST**
test_time_spring_type2_dt_type2_prev: SUB: 2010-03-13 18:38:28 EST - P-0Y0M0DT7H38M27S = **2010-03-14 03:16:55 EDT**
test_time_spring_type2_dt_type2_st: SUB: 2010-03-14 00:10:20 EST - P-0Y0M0DT2H6M35S = **2010-03-14 03:16:55 EDT**
test_time_spring_type2_dt_type2_dt: SUB: 2010-03-14 05:19:56 EDT - P+0Y0M0DT2H3M1S = **2010-03-14 03:16:55 EDT**
test_time_spring_type2_dt_type2_post: SUB: 2010-03-15 19:59:59 EDT - P+0Y0M1DT16H43M4S = **2010-03-14 03:16:55 EDT**
test_time_spring_type2_post_type2_prev: SUB: 2010-03-13 18:38:28 EST - P-0Y0M2DT1H21M31S = **2010-03-15 19:59:59 EDT**
test_time_spring_type2_post_type2_st: SUB: 2010-03-14 00:10:20 EST - P-0Y0M1DT18H49M39S = **2010-03-15 19:59:59 EDT**
test_time_spring_type2_post_type2_dt: SUB: 2010-03-14 03:16:55 EDT - P-0Y0M1DT16H43M4S = **2010-03-15 19:59:59 EDT**
test_time_spring_type2_post_type2_post: SUB: 2010-03-15 19:59:59 EDT - P+0Y0M0DT1H2M4S = **2010-03-15 18:57:55 EDT**

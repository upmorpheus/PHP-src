<?php

/* TODO: Convert "uninitialized" into an exception. This will remove many
 * possibilities for false return values in here. */

/** @return int|false */
function strtotime(string $time, int $now = UNKNOWN) {}

function date(string $format, int $timestamp = UNKNOWN): string {}

/** @return int|false */
function idate(string $format, int $timestamp = UNKNOWN) {}

function gmdate(string $format, int $timestamp = UNKNOWN): string {}

/** @return int|false */
function mktime(
    int $hour, int $min = UNKNOWN, int $sec = UNKNOWN,
    int $mon = UNKNOWN, int $day = UNKNOWN, int $year = UNKNOWN) {}

/** @return int|false */
function gmmktime(
    int $hour, int $min = UNKNOWN, int $sec = UNKNOWN,
    int $mon = UNKNOWN, int $day = UNKNOWN, int $year = UNKNOWN) {}

function checkdate(int $m, int $d, int $y): bool {}

/** @return string|false */
function strftime(string $format, int $timestamp = UNKNOWN) {}

/** @return string|false */
function gmstrftime(string $format, int $timestamp = UNKNOWN) {}

function time(): int {}

function localtime(int $timestamp = UNKNOWN, bool $associative = false): array {}

function getdate(int $timestamp = UNKNOWN): array {}

/** @return DateTime|false */
function date_create(string $time, ?DateTimeZone $timezone = null) {}

/** @return DateTime|false */
function date_create_immutable(string $time, ?DateTimeZone $timezone = null) {}

/** @return DateTime|false */
function date_create_from_format(string $format, string $time, ?DateTimeZone $timezone = null) {}

/** @return DateTimeImmutable|false */
function date_create_immutable_from_format(
    string $format, string $time, ?DateTimeZone $timezone = null) {}

function date_parse(string $date): array {}

function date_parse_from_format(string $format, string $date): array {}

/** @return array|false */
function date_get_last_errors() {}

/** @return string|false */
function date_format(DateTimeInterface $object, string $format) {}

/** @return DateTime|false */
function date_modify(DateTime $object, string $modify) {}

/** @return DateTime|false */
function date_add(DateTime $object, DateInterval $interval) {}

/** @return DateTime|false */
function date_sub(DateTime $object, DateInterval $interval) {}

/** @return DateTimeZone|false */
function date_timezone_get(DateTimeInterface $object) {}

/** @return DateTime|false */
function date_timezone_set(DateTimeInterface $object, DateTimeZone $timezone) {}

/** @return int|false */
function date_offset_get(DateTimeInterface $object) {}

/** @return DateInterval|false */
function date_diff(DateTimeInterface $object, DateTimeInterface $object2, bool $absolute = false) {}

/** @return DateTime|false */
function date_time_set(
    DateTime $object, int $hour, int $minute, int $second = 0, int $microseconds = 0) {}

/** @return DateTime|false */
function date_date_set(DateTime $object, int $year, int $month, int $day) {}

/** @return DateTime|false */
function date_isodate_set(DateTime $object, int $year, int $week, int $day = 1) {}

/** @return DateTime|false */
function date_timestamp_set(DateTime $object, int $timestamp) {}

/** @return int|false */
function date_timestamp_get(DateTimeInterface $object) {}

/** @return DateTimeZone|false */
function timezone_open(string $timezone) {}

/** @return string|false */
function timezone_name_get(DateTimeZone $object) {}

/** @return string|false */
function timezone_name_from_abbr(string $abbr, int $gmtoffset = -1, int $isdst = -1) {}

/** @return int|false */
function timezone_offset_get(DateTimeZone $object, DateTimeInterface $datetime) {}

/** @return array|false */
function timezone_transitions_get(
    DateTimeZone $object, int $timestamp_begin = PHP_INT_MIN, int $timestamp_end = PHP_INT_MAX) {}

/** @return array|false */
function timezone_location_get(DateTimeZone $object) {}

/** @return array|false */
function timezone_identifiers_list(int $what = DateTimeZone::ALL, ?string $country = null) {}

function timezone_abbreviations_list(): array {}

function timezone_version_get(): string {}

/** @return DateInterval|false */
function date_interval_create_from_date_string(string $time) {}

/** @return string|false */
function date_interval_format(DateInterval $object, string $format) {}

function date_default_timezone_set(string $timezone_identifier): bool {}

function date_default_timezone_get(): string {}

/** @return string|int|float|false */
function date_sunrise(
    int $time, int $retformat = SUNFUNCS_RET_STRING,
    float $latitude = UNKNOWN, float $longitude = UNKNOWN, float $zenith = UNKNOWN,
    float $gmt_offset = 0) {}

/** @return string|int|float|false */
function date_sunset(
    int $time, int $retformat = SUNFUNCS_RET_STRING,
    float $latitude = UNKNOWN, float $longitude = UNKNOWN, float $zenith = UNKNOWN,
    float $gmt_offset = 0) {}

function date_sun_info(int $time, float $latitude, float $longitude): array {}

// NB: Adding return types to methods is a BC break!
// For now only using @return annotations here.

interface DateTimeInterface {
    /** @return string */
    public function format(string $format);

    /** @return DateTimeZone|false */
    public function getTimezone();

    /** @return int|false */
    public function getOffset();

    /** @return int|false */
    public function getTimestamp();

    /** @return DateInterval|false */
    public function diff(DateTimeInterface $object, bool $absolute = false);

    public function __wakeup();
}

class DateTime implements DateTimeInterface {
    public function __construct(string $time, ?DateTimeZone $timezone = null);

    /** @return DateTime */
    public static function __set_state(array $array);

    /** @return DateTime */
    public static function createFromImmutable(DateTimeImmutable $object);

    /** @return DateTime|false */
    public static function createFromFormat(
        string $format, string $time, ?DateTimeZone $timezone = null);

    /** @return array|false */
    public static function getLastErrors();

    /** @return DateTime|false */
    public function modify(string $modify);

    /** @return DateTime|false */
    public function add(DateInterval $interval);

    /** @return DateTime|false */
    public function sub(DateInterval $interval);

    /** @return DateTime|false */
    public function setTimezone(DateTimeZone $timezone);

    /** @return DateTime|false */
    public function setTime(int $hour, int $minute, int $second = 0, int $microseconds = 0);

    /** @return DateTime|false */
    public function setDate(int $year, int $month, int $day);

    /** @return DateTime|false */
    public function setISODate(int $year, int $week, int $day = 1);

    /** @return DateTime|false */
    public function setTimestamp(int $timestampt);
}

class DateTimeImmutable implements DateTimeInterface {
    public function __construct(string $time, ?DateTimeZone $timezone = null);

    /** @return DateTimeZone */
    public static function __set_state();

    /** @return DateTimeImmutable */
    public static function createFromMutable(DateTime $object);

    /** @return DateTimeImmutable|false */
    public function modify(string $modify);

    /** @return DateTimeImmutable|false */
    public function add(DateInterval $interval);

    /** @return DateTimeImmutable|false */
    public function sub(DateInterval $interval);

    /** @return DateTimeImmutable|false */
    public function setTimezone(DateTimeZone $timezone);

    /** @return DateTimeImmutable|false */
    public function setTime(int $hour, int $minute, int $second = 0, int $microseconds = 0);

    /** @return DateTimeImmutable|false */
    public function setDate(int $year, int $month, int $day);

    /** @return DateTimeImmutable|false */
    public function setISODate(int $year, int $week, int $day = 1);

    /** @return DateTimeImmutable|false */
    public function setTimestamp(int $timestampt);
}

class DateTimeZone {
    public function __construct(string $timezone);

    /** @return string|false */
    public function getName();

    /** @return int|false */
    public function getOffset(DateTimeInterface $datetime);

    /** @return array|false */
    public function getTransitions(
        int $timestamp_begin = PHP_INT_MIN, int $timestamp_end = PHP_INT_MAX);

    /** @return array|false */
    public function getLocation();

    /** @return array */
    public static function listAbbreviations();

    /** @return array|false */
    public static function listIdentifiers(int $what = DateTimeZone::ALL, ?string $country = null);

    public function __wakeup();

    /** @return DateTimeZone */
    public static function __set_state();
}

class DateInterval {
    public function __construct(string $interval_spec);

    /** @return DateInterval|false */
    public static function createFromDateString(string $time);

    /** @return string|false */
    public function format(string $format);

    public function __wakeup();

    /** @return DateInterval */
    public static function __set_state(array $array);
}

class DatePeriod implements Traversable {
    /* Has an overloaded signature */
    public function __construct($start, $interval = UNKNOWN, $end = UNKNOWN);

    /** @return DateTimeInterface */
    public function getStartDate();

    /** @return DateTimeInterface|null */
    public function getEndDate();

    /** @return DateInterval */
    public function getDateInterval();

    /** @return int|null */
    public function getRecurrences();

    public function __wakeup();

    /** @return DatePeriod */
    public static function __set_state(array $array);
}

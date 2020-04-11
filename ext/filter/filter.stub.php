<?php

/** @generate-function-entries */

function filter_has_var(int $type, string $variable_name): bool {}

/**
 * @param mixed $options
 * @return mixed
 */
function filter_input(int $type, string $variable_name, int $filter = FILTER_DEFAULT, $options = null) {}

/**
 * @param mixed $variable
 * @param mixed $options
 * @return mixed
 */
function filter_var($variable, int $filter = FILTER_DEFAULT, $options = null) {}

/**
 * @param mixed $options
 * @return mixed
 */
function filter_input_array(int $type, $options = null, bool $add_empty = true) {}

/**
 * @param mixed $options
 * @return mixed
 */
function filter_var_array(array $data, $options = null, bool $add_empty = true) {}

function filter_list(): array {}

function filter_id(string $filtername): int|false {}

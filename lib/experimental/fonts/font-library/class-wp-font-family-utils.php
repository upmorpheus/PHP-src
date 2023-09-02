<?php
/**
 * Fonts Family Utils class.
 *
 * This file contains utils fot Font Family class.
 *
 * @package    WordPress
 * @subpackage Font Library
 * @since      6.4.0
 */

if ( class_exists( 'WP_Font_Family_Utils' ) ) {
	return;
}

/**
 * A class of utilities for working with the Font Library.
 *
 * @since 6.4.0
 */
class WP_Font_Family_Utils {

	/**
	 * Generates a filename for a font face asset.
	 *
	 * Creates a filename for a font face asset using font family, style, weight and
	 * extension information.
	 *
	 * @since 6.4.0
	 *
	 * @param string $font_slug The font slug to use in the filename.
	 * @param array  $font_face The font face array containing 'fontFamily', 'fontStyle', and
	 *                          'fontWeight' attributes.
	 * @param string $url       The URL of the font face asset, used to derive the file extension.
	 * @param string $suffix    Optional. The suffix added to the resulting filename. Default empty string.
	 * @return string The generated filename for the font face asset.
	 */
	public static function get_filename_from_font_face( $font_slug, $font_face, $url, $suffix = '' ) {
		$extension = pathinfo( $url, PATHINFO_EXTENSION );
		$filename  = "{$font_slug}_{$font_face['fontStyle']}_{$font_face['fontWeight']}";
		if ( '' !== $suffix ) {
			$filename .= "_{$suffix}";
		}

		return sanitize_file_name( "{$filename}.{$extension}" );
	}

	/**
	 * Merges two fonts and their font faces.
	 *
	 * @since 6.4.0
	 *
	 * @param array $font1 The first font to merge.
	 * @param array $font2 The second font to merge.
	 * @return array|WP_Error The merged font or WP_Error if the fonts have different slugs.
	 */
	public static function merge_fonts_data( $font1, $font2 ) {
		if ( $font1['slug'] !== $font2['slug'] ) {
			return new WP_Error(
				'fonts_must_have_same_slug',
				__( 'Fonts must have the same slug to be merged.', 'gutenberg' )
			);
		}

		$font_faces_1      = isset( $font1['fontFace'] ) ? $font1['fontFace'] : array();
		$font_faces_2      = isset( $font2['fontFace'] ) ? $font2['fontFace'] : array();
		$merged_font_faces = array_merge( $font_faces_1, $font_faces_2 );

		$serialized_faces        = array_map( 'serialize', $merged_font_faces );
		$unique_serialized_faces = array_unique( $serialized_faces );
		$unique_faces            = array_map( 'unserialize', $unique_serialized_faces );

		$merged_font             = array_merge( $font1, $font2 );
		$merged_font['fontFace'] = $unique_faces;

		return $merged_font;
	}

	/**
	 * Returns whether the given file has a font MIME type.
	 *
	 * @since 6.4.0
	 *
	 * @param string $filepath The file to check.
	 * @return bool True if the file has a font MIME type, false otherwise.
	 */
	public static function has_font_mime_type( $filepath ) {
		$filetype = wp_check_filetype( $filepath, WP_Font_Library::ALLOWED_FONT_MIME_TYPES );

		return in_array( $filetype['type'], WP_Font_Library::ALLOWED_FONT_MIME_TYPES, true );
	}
}

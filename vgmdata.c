#include <stdio.h>
#include <assert.h>

#include "TinyVGM/TinyVGM.h"

int callback_command(void *p, unsigned int cmd, const void *buf, uint32_t len);
int callback_header(void *userp, TinyVGMHeaderField field, uint32_t value);
int callback_metadata(void *p, TinyVGMMetadataType type, uint32_t file_offset, uint32_t len);
int callback_datablock(void *p, unsigned int type, uint32_t file_offset, uint32_t len);
int read_callback(void *p, uint8_t *buf, uint32_t len);
int seek_callback(void *p, uint32_t pos);

const char *vgm_datablock_type_to_string(unsigned int type);

uint32_t gd3_offset_abs = 0, data_offset_abs = 0;

int main(int argc, char **argv) {
	if(!argv[1]) {
		puts("Usage: vgmdata <file.vgm>");
		exit(-1);
	}

	FILE *file = fopen(argv[1], "rb");
	assert(file);

	TinyVGMContext ctx = {
		.callback = {
			.header = callback_header,
			.metadata = callback_metadata,
			.command = callback_command,
			.data_block = callback_datablock,
			.seek = seek_callback,
			.read = read_callback
		},
		.userp = file
	};

	int ret = tinyvgm_parse_header(&ctx);

	printf("tinyvgm_parse_header returned %d\n", ret);

	if(ret == TinyVGM_OK) {
		if(gd3_offset_abs) {
			ret = tinyvgm_parse_metadata(&ctx, gd3_offset_abs);
			printf("tinyvgm_parse_metadata returned %d\n", ret);
		}

		ret = tinyvgm_parse_commands(&ctx, data_offset_abs);
		printf("tinyvgm_parse_commands returned %d\n", ret);
	}

	fclose(file);

	return 0;
}

int callback_command(void *p, unsigned int cmd, const void *buf, uint32_t len) {
	switch(cmd) {
		case 0x90:
		case 0x91:
		case 0x92:
		case 0x93:
		case 0x94:
		case 0x95:
		case 0xB7:
		case 0xB8:
			printf("Command: cmd=0x%02x, len=%" PRIu32 ", data:", cmd, len);

			for(uint32_t i = 0; i < len; i++)
				printf("%02x ", ((uint8_t *)buf)[i]);
			puts("");
			return TinyVGM_OK;
		default:
			return TinyVGM_OK;
}

int callback_header(void *userp, TinyVGMHeaderField field, uint32_t value) {

	switch (field) {
		case TinyVGM_HeaderField_Version:
			if (value < 0x00000150) {
				data_offset_abs = 0x40;
				printf("Pre-1.50 version detected, Data Offset is 0x40\n");
			}
			break;
		case TinyVGM_HeaderField_GD3_Offset:
			gd3_offset_abs = value + tinyvgm_headerfield_offset(field);
			printf("GD3 ABS Offset: 0x%08" PRIx32 " (%" PRIu32 ")\n", gd3_offset_abs, gd3_offset_abs);
			break;
		case TinyVGM_HeaderField_Data_Offset:
			data_offset_abs = value + tinyvgm_headerfield_offset(field);
			printf("Data Offset: 0x%08" PRIx32 " (%" PRIu32 ")\n", data_offset_abs, data_offset_abs);
			break;
		default:
			break;
	}


	return TinyVGM_OK;
}

int callback_metadata(void *p, TinyVGMMetadataType type, uint32_t file_offset, uint32_t len) {
	printf("Metadata: Type=%u, FileOffset=%" PRIu32 ", Len=%" PRIu32 ", ", type, file_offset, len);

	FILE *fp = p;
	long cur_pos = ftell(fp);
	fseek(fp, file_offset, SEEK_SET);

	for (size_t i=0; i<len; i+=2) {
		uint16_t c;
		fread(&c, 1, 2, fp);
		if (c > 127) {
			c = '?';
		}
		printf("%c", c);
	}

	puts("");
	fseek(fp, cur_pos, SEEK_SET);

	return TinyVGM_OK;
}

int callback_datablock(void *p, unsigned int type, uint32_t file_offset, uint32_t len) {
	printf("Data block: Type=%s, FileOffset=%" PRIu32 ", Len=%" PRIu32 "\n", vgm_datablock_type_to_string(type), file_offset, len);

	/*
	FILE *fp = p;
	long cur_pos = ftell(fp);
	fseek(fp, file_offset, SEEK_SET);

	for (size_t i=0; i<len; i++) {
		uint8_t c;
		fread(&c, 1, 1, fp);
		printf("%02x ", c);
	}

	puts("");
	fseek(fp, cur_pos, SEEK_SET);
	*/

	return TinyVGM_OK;
}

int read_callback(void *p, uint8_t *buf, uint32_t len) {
	size_t ret = fread(buf, 1, len, (FILE*)p);

	if(ret) {
		return (int32_t)ret;
	} else {
		return feof((FILE*)p) ? 0 : TinyVGM_EIO;
	}
}

int seek_callback(void *p, uint32_t pos) {
	return fseek((FILE*)p, pos, SEEK_SET);
}

const char *vgm_datablock_type_to_string(unsigned int type) {
	switch (type) {
		case 0x00:
			return "YM2612 PCM data";
		case 0x01:
			return "RF5C68 PCM data";
		case 0x02:
			return "RF5C164 PCM data";
		case 0x03:
			return "PWM PCM data";
		case 0x04:
			return "OKIM6258 ADPCM data";
		case 0x05:
			return "HuC6280 PCM data";
		case 0x06:
			return "SCSP PCM data";
		case 0x07:
			return "NES APU DPCM data";
		case 0x40:
			return "Compressed YM2612 PCM data";
		case 0x41:
			return "Compressed RF5C68 PCM data";
		case 0x42:
			return "Compressed RF5C164 PCM data";
		case 0x43:
			return "Compressed PWM PCM data";
		case 0x44:
			return "Compressed OKIM6258 ADPCM data";
		case 0x45:
			return "HuC6280 PCM data";
		case 0x46:
			return "SCSP PCM data";
		case 0x47:
			return "NES APU DPCM data";
		case 0x80:
			return "Sega PCM ROM data";
		case 0x81:
			return "YM2608 DELTA-T ROM data";
		case 0x82:
			return "YM2610 ADPCM ROM data";
		case 0x83:
			return "YM2610 DELTA-T ROM data";
		case 0x84:
			return "YMF278B ROM data";
		case 0x85:
			return "YMZ271 ROM data";
		case 0x86:
			return "YMF280B ROM data";
		case 0x87:
			return "YMF278B RAM data";
		case 0x88:
			return "Y8950 DELTA-T ROM data";
		case 0x89:
			return "MultiPCM ROM data";
		case 0x8A:
			return "uPD7759 ROM data";
		case 0x8B:
			return "OKIM6295 ROM data";
		case 0x8C:
			return "K054539 ROM data";
		case 0x8D:
			return "C140 ROM data";
		case 0x8E:
			return "K053260 ROM data";
		case 0x8F:
			return "Q-Sound ROM data";
		case 0x90:
			return "ES5505/ES5506 ROM data";
		case 0x91:
		        return "X1-010 ROM data";
		case 0x92:
		        return "C352 ROM data";
		case 0x93:
			return "GA20 ROM data";
		case 0xC0:
			return "RF5C68 RAM write";
		case 0xC1:
			return "RF5C164 RAM write";
		case 0xC2:
			return "NES APU RAM write";
		case 0xE0:
			return "SCSP RAM write";
		case 0xE1:
			return "ES5503 RAM write";
		default:
			return "";
	}
}

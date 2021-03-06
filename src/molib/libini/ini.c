#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/file.h>
#include <errno.h>
#include <unistd.h>

#include <fcntl.h>                          // 提供open()函数  
//#include <sys/types.h>                      // 提供mode_t类型  
#include <sys/stat.h>                       // 提供open()函数的符号  




#include "ini_interface.h"

#define MAX_CFG_BUF 4096

#define INIRW_LOCK       "/var/run/inirw.lock"

/* 椤规爣蹇楃鍓嶅悗缂� --鍙牴鎹壒娈婇渶瑕佽繘琛屽畾涔夋洿鏀癸紝濡倇 }绛� */
const char section_prefix = '[';
const char section_suffix = ']';

/* 娉ㄩ噴绗�,瀛楃涓蹭腑浠讳綍涓�涓兘鍙互浣滀负娉ㄩ噴绗﹀彿 */
const char *ini_comments = ";#";

int CFG_section_line_no, CFG_key_line_no, CFG_key_lines;

static char *strtrimr(char *buf);
static char *strtriml(char *buf);
static int FileGetLine(FILE * fp, char *buffer, int maxlen);
static int SplitKeyValue(char *buf, char **key, char **val);
static int FileCopy(char *source_file, char *dest_file);

static int inirw_lock(void)
{
#ifndef INI_FOR_HOST
    int ret = -1;
    int inirw_lock_fd = -1;

    inirw_lock_fd = open(INIRW_LOCK, O_RDWR | O_CREAT,
                            S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    if (inirw_lock_fd < 0) {
        fprintf(stderr, "%s: open %s fail: %s\n", __func__, INIRW_LOCK, strerror(errno));
        return inirw_lock_fd;
    }

    ret = flock(inirw_lock_fd, LOCK_EX);
    if (ret) {
        fprintf(stderr, "%s: flock failed.\n", __func__);
        close(inirw_lock_fd);
        inirw_lock_fd = -1;
        return inirw_lock_fd;
    }

    return inirw_lock_fd;
#else
    return -2;
#endif
}

static void inirw_unlock(int lock)
{
#ifndef INI_FOR_HOST
    if (lock >= 0)
        close(lock);
#endif

    return;
}

/**********************************************************************
 * 鍑芥暟鍚嶇О锛� strtrimr
 * 鍔熻兘鎻忚堪锛� 鍘婚櫎瀛楃涓插彸杈圭殑绌哄瓧绗�
 * 璁块棶鐨勮〃锛� 鏃�
 * 淇敼鐨勮〃锛� 鏃�
 * 杈撳叆鍙傛暟锛� char * buf 瀛楃涓叉寚閽�
 * 杈撳嚭鍙傛暟锛� 鏃�
 * 杩� 鍥� 鍊硷細 瀛楃涓叉寚閽�
 * 鍏跺畠璇存槑锛� 鏃�
 ***********************************************************************/
static char *strtrimr(char *buf)
{
	int len, i;
	char *tmp = NULL;
	len = strlen(buf);
	tmp = (char *)malloc(len);

	memset(tmp, 0x00, len);
	for (i = 0; i < len; i++) {
		if (buf[i] != ' ')
			break;
	}
	if (i < len) {
		strncpy(tmp, (buf + i), (len - i));
	}
	strncpy(buf, tmp, len);
	free(tmp);
	return buf;
}

/**********************************************************************
 * 鍑芥暟鍚嶇О锛� strtriml
 * 鍔熻兘鎻忚堪锛� 鍘婚櫎瀛楃涓插乏杈圭殑绌哄瓧绗�
 * 璁块棶鐨勮〃锛� 鏃�
 * 淇敼鐨勮〃锛� 鏃�
 * 杈撳叆鍙傛暟锛� char * buf 瀛楃涓叉寚閽�
 * 杈撳嚭鍙傛暟锛� 鏃�
 * 杩� 鍥� 鍊硷細 瀛楃涓叉寚閽�
 * 鍏跺畠璇存槑锛� 鏃�
 ***********************************************************************/
static char *strtriml(char *buf)
{
	int len, i;
	char *tmp = NULL;
	len = strlen(buf);
	tmp = (char *)malloc(len);
	memset(tmp, 0x00, len);
	for (i = 0; i < len; i++) {
		if (buf[len - i - 1] != ' ')
			break;
	}
	if (i < len) {
		strncpy(tmp, buf, len - i);
	}
	strncpy(buf, tmp, len);
	free(tmp);
	return buf;
}

/**********************************************************************
 * 鍑芥暟鍚嶇О锛� FileGetLine
 * 鍔熻兘鎻忚堪锛� 浠庢枃浠朵腑璇诲彇涓�琛�
 * 璁块棶鐨勮〃锛� 鏃�
 * 淇敼鐨勮〃锛� 鏃�
 * 杈撳叆鍙傛暟锛� FILE *fp 鏂囦欢鍙ユ焺锛沬nt maxlen 缂撳啿鍖烘渶澶ч暱搴�
 * 杈撳嚭鍙傛暟锛� char *buffer 涓�琛屽瓧绗︿覆
 * 杩� 鍥� 鍊硷細 瀹為檯璇荤殑闀垮害
 * 鍏跺畠璇存槑锛� 鏃�
 ***********************************************************************/
static int FileGetLine(FILE * fp, char *buffer, int maxlen)
{
	int i, j;
	char ch1;

	for (i = 0, j = 0; i < maxlen; j++) {
		if (fread(&ch1, sizeof(char), 1, fp) != 1) {
			if (feof(fp) != 0) {
				if (j == 0)
					return -1;	/* 鏂囦欢缁撴潫 */
				else
					break;
			}
			if (ferror(fp) != 0)
				return -2;	/* 璇绘枃浠跺嚭閿� */
			return -2;
		} else {
			if (ch1 == '\n' || ch1 == 0x00)
				break;	/* 鎹㈣ */
			if (ch1 == '\f' || ch1 == 0x1A) {	/* '\f':鎹㈤〉绗︿篃绠楁湁鏁堝瓧绗� */
				buffer[i++] = ch1;
				break;
			}
			if (ch1 != '\r')
				buffer[i++] = ch1;	/* 蹇界暐鍥炶溅绗� */
		}
	}
	buffer[i] = '\0';
	return i;
}

/**********************************************************************
 * 鍑芥暟鍚嶇О锛� FileCopy
 * 鍔熻兘鎻忚堪锛� 鏂囦欢鎷疯礉
 * 璁块棶鐨勮〃锛� 鏃�
 * 淇敼鐨勮〃锛� 鏃�
 * 杈撳叆鍙傛暟锛� void *source_file銆�婧愭枃浠躲��void *dest_file銆�鐩爣鏂囦欢
 * 杈撳嚭鍙傛暟锛� 鏃�
 * 杩� 鍥� 鍊硷細 0 -- OK,闈�0锛嶏紞澶辫触
 * 鍏跺畠璇存槑锛� 鏃�
 ***********************************************************************/
static int FileCopy(char *source_file, char *dest_file)
{
	FILE *fp1, *fp2;
	char buf[1024 + 1];
	int ret;

	if ((fp1 = fopen(source_file, "r")) == NULL)
		return COPYF_ERR_OPEN_FILE;
	ret = COPYF_ERR_CREATE_FILE;

	if ((fp2 = fopen(dest_file, "w")) == NULL)
		goto copy_end;

	while (1) {
		ret = COPYF_ERR_READ_FILE;
		memset(buf, 0x00, 1024 + 1);
		if (fgets(buf, 1024, fp1) == NULL) {
			if (strlen(buf) == 0) {
				if (ferror(fp1) != 0)
					goto copy_end;
				break;	/* 鏂囦欢灏� */
			}
		}
		ret = COPYF_ERR_WRITE_FILE;
		if (fputs(buf, fp2) == EOF)
			goto copy_end;
	}
	ret = COPYF_OK;
copy_end:
	if (fp2 != NULL)
		fclose(fp2);
	if (fp1 != NULL)
		fclose(fp1);
	return ret;
}

/**********************************************************************
 * 鍑芥暟鍚嶇О锛� SplitKeyValue
 * 鍔熻兘鎻忚堪锛� 鍒嗙key鍜寁alue
 *銆�銆�銆�銆�銆�銆�key=val
 *            jack = liaoyuewang
 *             |   |   |
 *             k1  k2  i
 * 璁块棶鐨勮〃锛� 鏃�
 * 淇敼鐨勮〃锛� 鏃�
 * 杈撳叆鍙傛暟锛� char *buf
 * 杈撳嚭鍙傛暟锛� char **key;char **val
 * 杩� 鍥� 鍊硷細 1 --- ok
 *             0 --- blank line
 *            -1 --- no key, "= val"
 *            -2 --- only key, no '='
 * 鍏跺畠璇存槑锛� 鏃�
 ***********************************************************************/
static int SplitKeyValue(char *buf, char **key, char **val)
{
	int i, k1, k2, n;

	if ((n = strlen((char *)buf)) < 1)
		return 0;
	for (i = 0; i < n; i++)
		if (buf[i] != ' ' && buf[i] != '\t')
			break;
	if (i >= n)
		return 0;
	if (buf[i] == '=')
		return -1;
	k1 = i;
	for (i++; i < n; i++)
		if (buf[i] == '=')
			break;
	if (i >= n)
		return -2;
	k2 = i;
	for (i++; i < n; i++)
		if (buf[i] != ' ' && buf[i] != '\t')
			break;
	buf[k2] = '\0';
	*key = buf + k1;
	*val = buf + i;
	return 1;
}

/**********************************************************************
 * 鍑芥暟鍚嶇О锛� mozart_ini_getkey
 * 鍔熻兘鎻忚堪锛� 鑾峰緱key鐨勫��
 * 璁块棶鐨勮〃锛� 鏃�
 * 淇敼鐨勮〃锛� 鏃�
 * 杈撳叆鍙傛暟锛� char *ini_file銆�鏂囦欢锛沜har *section銆�椤瑰�硷紱char *key銆�閿��
 * 杈撳嚭鍙傛暟锛� char *value key鐨勫��
 * 杩� 鍥� 鍊硷細 0 --- ok 闈�0 --- error
 * 鍏跺畠璇存槑锛� 鏃�
 ***********************************************************************/
int _mozart_ini_getkey(char *ini_file, char *section, char *key, char *value)
{
	FILE *fp;
	char buf1[MAX_CFG_BUF + 1], buf2[MAX_CFG_BUF + 1];
	char *key_ptr, *val_ptr;
	int line_no, n, ret;

	line_no = 0;
	CFG_section_line_no = 0;
	CFG_key_line_no = 0;
	CFG_key_lines = 0;

	if ((fp = fopen(ini_file, "rb")) == NULL) {
		return CFG_ERR_OPEN_FILE;
	}

	while (1) {		/* 鎼滄壘椤箂ection */
		ret = CFG_ERR_READ_FILE;
		n = FileGetLine(fp, buf1, MAX_CFG_BUF);
		if (n < -1)
			goto r_cfg_end;
		ret = CFG_SECTION_NOT_FOUND;
		if (n < 0)
			goto r_cfg_end;	/* 鏂囦欢灏撅紝鏈彂鐜� */
		line_no++;
		n = strlen(strtriml(strtrimr(buf1)));
		if (n == 0 || strchr(ini_comments, buf1[0]))
			continue;	/* 绌鸿 鎴� 娉ㄩ噴琛� */
		ret = CFG_ERR_FILE_FORMAT;
		if (n > 2
		    &&
		    ((buf1[0] == section_prefix
		      && buf1[n - 1] != section_suffix)))
			goto r_cfg_end;
		if (buf1[0] == section_prefix) {
			buf1[n - 1] = 0x00;
			if (strcmp(buf1 + 1, section) == 0)
				break;	/* 鎵惧埌椤箂ection */
		}
	}
	CFG_section_line_no = line_no;
	while (1) {		/* 鎼滄壘key */
		ret = CFG_ERR_READ_FILE;
		n = FileGetLine(fp, buf1, MAX_CFG_BUF);
		if (n < -1)
			goto r_cfg_end;
		ret = CFG_KEY_NOT_FOUND;
		if (n < 0)
			goto r_cfg_end;	/* 鏂囦欢灏撅紝鏈彂鐜発ey */
		line_no++;
		CFG_key_line_no = line_no;
		CFG_key_lines = 1;
		n = strlen(strtriml(strtrimr(buf1)));
		if (n == 0 || strchr(ini_comments, buf1[0]))
			continue;	/* 绌鸿 鎴� 娉ㄩ噴琛� */
		ret = CFG_KEY_NOT_FOUND;
		if (buf1[0] == section_prefix)
			goto r_cfg_end;
		if (buf1[n - 1] == '\\') {	/* 閬嘰鍙疯〃绀轰笅涓�琛岀户缁� */
			buf1[n - 1] = 0x00;
			while (1) {
				ret = CFG_ERR_READ_FILE;
				n = FileGetLine(fp, buf2, MAX_CFG_BUF);
				if (n < -1)
					goto r_cfg_end;
				if (n < 0)
					break;	/* 鏂囦欢缁撴潫 */
				line_no++;
				CFG_key_lines++;
				n = strlen(strtrimr(buf2));
				ret = CFG_ERR_EXCEED_BUF_SIZE;
				if (n > 0 && buf2[n - 1] == '\\') {	/* 閬嘰鍙疯〃绀轰笅涓�琛岀户缁� */
					buf2[n - 1] = 0x00;
					if (strlen(buf1) + strlen(buf2) >
					    MAX_CFG_BUF)
						goto r_cfg_end;
					strcat(buf1, buf2);
					continue;
				}
				if (strlen(buf1) + strlen(buf2) > MAX_CFG_BUF)
					goto r_cfg_end;
				strcat(buf1, buf2);
				break;
			}
		}
		ret = CFG_ERR_FILE_FORMAT;
		if (SplitKeyValue(buf1, &key_ptr, &val_ptr) != 1)
			goto r_cfg_end;
		strtriml(strtrimr(key_ptr));
		if (strcmp(key_ptr, key) != 0)
			continue;	/* 鍜宬ey鍊间笉鍖归厤 */
		strcpy(value, val_ptr);
		break;
	}
	ret = CFG_OK;
r_cfg_end:
	if (fp != NULL)
		fclose(fp);
	return ret;
}


int mozart_ini_getkey(char *ini_file, char *section, char *key, char *value)
{
	int ret = -1;
	int lock = -1;

	if ((lock = inirw_lock()) == -1) {
		fprintf(stderr, "%s: inirw lock fail.\n", __func__);
		return -1;
	}

	ret = _mozart_ini_getkey(ini_file, section, key, value);

	inirw_unlock(lock);

	return ret;
}

/**********************************************************************
 * 鍑芥暟鍚嶇О锛� mozart_ini_setkey
 * 鍔熻兘鎻忚堪锛� 璁剧疆key鐨勫��
 * 璁块棶鐨勮〃锛� 鏃�
 * 淇敼鐨勮〃锛� 鏃�
 * 杈撳叆鍙傛暟锛� char *ini_file銆�鏂囦欢锛沜har *section銆�椤瑰�硷紱
 *              char *key銆�閿�硷紱char *value key鐨勫��
 * 杈撳嚭鍙傛暟锛� 鏃�
 * 杩� 鍥� 鍊硷細 0 --- ok 闈�0 --- error
 * 鍏跺畠璇存槑锛� 鏃�
 ***********************************************************************/
int mozart_ini_setkey(char *ini_file, char *section, char *key, char *value)
{
	FILE *fp1, *fp2;
	char buf1[MAX_CFG_BUF + 1];
	int line_no, line_no1, n, ret, ret2;
	char *tmpfname;
	int lock = -1;

	if ((lock = inirw_lock()) == -1) {
		fprintf(stderr, "%s: inirw lock fail.\n", __func__);
		return -1;
	}

	ret = _mozart_ini_getkey(ini_file, section, key, buf1);
	if (ret <= CFG_ERR && ret != CFG_ERR_OPEN_FILE)
		return ret;

	if (ret == CFG_ERR_OPEN_FILE || ret == CFG_SECTION_NOT_FOUND) {

		if ((fp1 = fopen((char *)ini_file, "a")) == NULL) {
			inirw_unlock(lock);
			return CFG_ERR_CREATE_FILE;
		}

		if (fprintf(fp1, "%c%s%c\n", section_prefix, section, section_suffix) == EOF) {
			fclose(fp1);
			inirw_unlock(lock);
			return CFG_ERR_WRITE_FILE;
		}
		if (fprintf(fp1, "%s=%s\n", key, value) == EOF) {
			fclose(fp1);
			inirw_unlock(lock);
			return CFG_ERR_WRITE_FILE;
		}
		fclose(fp1);
		inirw_unlock(lock);
		return CFG_OK;
	}
	if ((tmpfname = tmpnam(NULL)) == NULL) {
		inirw_unlock(lock);
		return CFG_ERR_CREATE_FILE;
	}

	if ((fp2 = fopen(tmpfname, "w")) == NULL) {
		inirw_unlock(lock);
		return CFG_ERR_CREATE_FILE;
	}
	ret2 = CFG_ERR_OPEN_FILE;

	if ((fp1 = fopen((char *)ini_file, "rb")) == NULL)
		goto w_cfg_end;

	if (ret == CFG_KEY_NOT_FOUND)
		line_no1 = CFG_section_line_no;
	else			/* ret = CFG_OK */
		line_no1 = CFG_key_line_no - 1;
	for (line_no = 0; line_no < line_no1; line_no++) {
		ret2 = CFG_ERR_READ_FILE;
		n = FileGetLine(fp1, buf1, MAX_CFG_BUF);
		if (n < 0)
			goto w_cfg_end;
		ret2 = CFG_ERR_WRITE_FILE;
		if (fprintf(fp2, "%s\n", buf1) == EOF)
			goto w_cfg_end;
	}
	if (ret != CFG_KEY_NOT_FOUND)
		for (; line_no < line_no1 + CFG_key_lines; line_no++) {
			ret2 = CFG_ERR_READ_FILE;
			n = FileGetLine(fp1, buf1, MAX_CFG_BUF);
			if (n < 0)
				goto w_cfg_end;
		}
	ret2 = CFG_ERR_WRITE_FILE;
	if (fprintf(fp2, "%s=%s\n", key, value) == EOF)
		goto w_cfg_end;
	while (1) {
		ret2 = CFG_ERR_READ_FILE;
		n = FileGetLine(fp1, buf1, MAX_CFG_BUF);
		if (n < -1)
			goto w_cfg_end;
		if (n < 0)
			break;
		ret2 = CFG_ERR_WRITE_FILE;
		if (fprintf(fp2, "%s\n", buf1) == EOF)
			goto w_cfg_end;
	}
	ret2 = CFG_OK;
w_cfg_end:
	if (fp1 != NULL)
		fclose(fp1);
	if (fp2 != NULL)
		fclose(fp2);
	if (ret2 == CFG_OK) {
		ret = FileCopy(tmpfname, ini_file);
		if (ret != 0) {
			inirw_unlock(lock);
			return CFG_ERR_CREATE_FILE;
		}
	}
	remove(tmpfname);

	inirw_unlock(lock);
	return ret2;
}

/**********************************************************************
 * 鍑芥暟鍚嶇О锛� mozart_ini_getsections1
 * 鍔熻兘鎻忚堪锛� 鑾峰緱鎵�鏈塻ection
 * 璁块棶鐨勮〃锛� 鏃�
 * 淇敼鐨勮〃锛� 鏃�
 * 杈撳叆鍙傛暟锛� char *ini_file銆�鏂囦欢
 * 杈撳嚭鍙傛暟锛� int *sections銆�瀛樻斁section涓暟
 * 杩� 鍥� 鍊硷細 鎵�鏈塻ection锛屽嚭閿欒繑鍥濶ULL
 * 鍏跺畠璇存槑锛� 鏃�
 ***********************************************************************/
char **mozart_ini_getsections1(char *ini_file, int *n_sections)
{
	FILE *fp = NULL;
	char buf1[MAX_CFG_BUF + 1] = {};
	int i = 0;
	int j = 0;
	int n = 0;
	char **sections = NULL;

	if ((fp = fopen(ini_file, "rb")) == NULL)
		return NULL;

	while (1) {		/*鎼滄壘椤箂ection */
		n = FileGetLine(fp, buf1, MAX_CFG_BUF);
		if (n < -1)
			goto cfg_scts_end;
		if (n < 0)
			break;	/* 鏂囦欢灏� */

		n = strlen(strtriml(strtrimr(buf1)));
		if (n == 0 || strchr(ini_comments, buf1[0]))
			continue;	/* 绌鸿 鎴� 娉ㄩ噴琛� */

		if (n > 2 && ((buf1[0] == section_prefix && buf1[n - 1] != section_suffix)))
			goto cfg_scts_end;

		if (buf1[0] == section_prefix) {
			buf1[n - 1] = '\0';

			i++;

			sections = realloc(sections, i * sizeof(char **));
			sections[i - 1] = malloc(strlen(buf1 + 1) + 1);

			strcpy(sections[i - 1], buf1 + 1);
		}
	}

	if (fp != NULL)
		fclose(fp);

	*n_sections = i;
	return sections;

cfg_scts_end:
	if (fp != NULL)
		fclose(fp);

	if (sections) {
		for (j = 0; j < i; j++)
			free(sections[j]);
		free(sections);
	}

	return NULL;
}


/**********************************************************************
 * 鍑芥暟鍚嶇О锛� mozart_ini_getsections
 * 鍔熻兘鎻忚堪锛� 鑾峰緱鎵�鏈塻ection
 * 璁块棶鐨勮〃锛� 鏃�
 * 淇敼鐨勮〃锛� 鏃�
 * 杈撳叆鍙傛暟锛� char *ini_file銆�鏂囦欢
 * 杈撳嚭鍙傛暟锛� char *sections[]銆�瀛樻斁section鍚嶅瓧
 * 杩� 鍥� 鍊硷細 杩斿洖section涓暟銆傝嫢鍑洪敊锛岃繑鍥炶礋鏁般��
 * 鍏跺畠璇存槑锛� 鏃�
 ***********************************************************************/
int mozart_ini_getsections(char *ini_file, char **sections)
{
	int i = 0;
	int n_sections = 0;
	char **ss = NULL;

	ss = mozart_ini_getsections1(ini_file, &n_sections);
	if (!ss)
		return 0;

	for (i = 0; i < n_sections; i++) {
		if (sections[i])
			strcpy(sections[i], ss[i]);
		free(ss[i]);
	}
	free(ss);

	return n_sections;
}

/**********************************************************************
 * 鍑芥暟鍚嶇О锛� mozart_ini_getkeys
 * 鍔熻兘鎻忚堪锛� 鑾峰緱鎵�鏈塳ey鐨勫悕瀛楋紙key=value褰㈠紡, value鍙敤鍔犲彿琛ㄧず缁锛�
 *                [section]
 *                name = al\
 *                ex
 *          绛変环浜�
 *                [section]
 *                name=alex
 * 璁块棶鐨勮〃锛� 鏃�
 * 淇敼鐨勮〃锛� 鏃�
 * 杈撳叆鍙傛暟锛� char *ini_file銆�鏂囦欢 char *section 椤瑰��
 * 杈撳嚭鍙傛暟锛� char *keys[]銆�瀛樻斁key鍚嶅瓧
 * 杩� 鍥� 鍊硷細 杩斿洖key涓暟銆傝嫢鍑洪敊锛岃繑鍥炶礋鏁般��
 * 鍏跺畠璇存槑锛� 鏃�
 ***********************************************************************/
int mozart_ini_getkeys(char *ini_file, char *section, char *keys[])
{
	FILE *fp;
	char buf1[MAX_CFG_BUF + 1], buf2[MAX_CFG_BUF + 1];
	char *key_ptr, *val_ptr;
	int n, n_keys = 0, ret;

	if ((fp = fopen(ini_file, "rb")) == NULL)
		return CFG_ERR_OPEN_FILE;

	while (1) {		/* 鎼滄壘椤箂ection */
		ret = CFG_ERR_READ_FILE;
		n = FileGetLine(fp, buf1, MAX_CFG_BUF);
		if (n < -1)
			goto cfg_keys_end;
		ret = CFG_SECTION_NOT_FOUND;
		if (n < 0)
			goto cfg_keys_end;	/* 鏂囦欢灏� */
		n = strlen(strtriml(strtrimr(buf1)));
		if (n == 0 || strchr(ini_comments, buf1[0]))
			continue;	/* 绌鸿 鎴� 娉ㄩ噴琛� */
		ret = CFG_ERR_FILE_FORMAT;
		if (n > 2
		    &&
		    ((buf1[0] == section_prefix
		      && buf1[n - 1] != section_suffix)))
			goto cfg_keys_end;
		if (buf1[0] == section_prefix) {
			buf1[n - 1] = 0x00;
			if (strcmp(buf1 + 1, section) == 0)
				break;	/* 鎵惧埌椤箂ection */
		}
	}
	while (1) {
		ret = CFG_ERR_READ_FILE;
		n = FileGetLine(fp, buf1, MAX_CFG_BUF);
		if (n < -1)
			goto cfg_keys_end;
		if (n < 0)
			break;	/* 鏂囦欢灏� */
		n = strlen(strtriml(strtrimr(buf1)));
		if (n == 0 || strchr(ini_comments, buf1[0]))
			continue;	/* 绌鸿 鎴� 娉ㄩ噴琛� */
		ret = CFG_KEY_NOT_FOUND;
		if (buf1[0] == section_prefix)
			break;	/* 鍙︿竴涓� section */
		if (buf1[n - 1] == '\\') {	/* 閬嘰鍙疯〃绀轰笅涓�琛岀户缁� */
			buf1[n - 1] = 0x00;
			while (1) {
				ret = CFG_ERR_READ_FILE;
				n = FileGetLine(fp, buf2, MAX_CFG_BUF);
				if (n < -1)
					goto cfg_keys_end;
				if (n < 0)
					break;	/* 鏂囦欢灏� */
				n = strlen(strtrimr(buf2));
				ret = CFG_ERR_EXCEED_BUF_SIZE;
				if (n > 0 && buf2[n - 1] == '\\') {	/* 閬嘰鍙疯〃绀轰笅涓�琛岀户缁� */
					buf2[n - 1] = 0x00;
					if (strlen(buf1) + strlen(buf2) >
					    MAX_CFG_BUF)
						goto cfg_keys_end;
					strcat(buf1, buf2);
					continue;
				}
				if (strlen(buf1) + strlen(buf2) > MAX_CFG_BUF)
					goto cfg_keys_end;
				strcat(buf1, buf2);
				break;
			}
		}
		ret = CFG_ERR_FILE_FORMAT;
		if (SplitKeyValue(buf1, &key_ptr, &val_ptr) != 1)
			goto cfg_keys_end;
		strtriml(strtrimr(key_ptr));
		strcpy(keys[n_keys], key_ptr);
		n_keys++;
	}
	ret = n_keys;
cfg_keys_end:
	if (fp != NULL)
		fclose(fp);
	return ret;
}

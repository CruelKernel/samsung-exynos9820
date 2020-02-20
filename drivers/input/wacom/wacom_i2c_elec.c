/*
 *  wacom_i2c_elec.c - Wacom G5 Digitizer Controller (I2C bus)
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/i2c.h>
#include "wacom.h"


long long median(long long* arr, int length)
{
	long long* internal;
	long long ret = -1;
	int i, j;
	long long temp;

	internal = kzalloc(sizeof(long long)*length, GFP_KERNEL);
	memcpy(internal, arr, length * sizeof(long long));

	for (i = 0; i < length; i++) {
		for (j = 0; j < length - (i + 1); j++) {
			if (internal[j] > internal[j + 1]) {
				temp = internal[j + 1];
				internal[j + 1] = internal[j];
				internal[j] = temp;
			}
		}
	}

	if (length % 2 == 0)
		ret = (internal[length / 2 - 1] + internal[length / 2]) / 2;
	else
		ret = internal[length / 2];

	kfree(internal);

	return ret;
}

long long buildS(long long* x, long long* m, int start, int end, int ind)
{
	int i;
	long long ret;
	long long *temp;

	temp = kzalloc(sizeof(long long)*(end - start + 1), GFP_KERNEL);
	memcpy(temp, &x[start], sizeof(long long)*(end - start + 1));
	for (i = 0; i < end - start + 1; i++) {
		temp[i] -= m[ind];
		temp[i] = (temp[i] > 0) ? temp[i] : -temp[i];
	}

	ret = median(temp, end - start + 1);

	kfree(temp);

	return ret;
}

void hampel(long long* x, int length, int k, int nsig)
{
	const long long kappa = 14826;
	int i;

	long long *m;
	long long *s;
	long long *temp;

	m = kzalloc(sizeof(long long)*length, GFP_KERNEL);
	s = kzalloc(sizeof(long long)*length, GFP_KERNEL);
	temp = kzalloc(sizeof(long long)*length, GFP_KERNEL);

	for (i = 0; i < length; i++) {
		if (i < k) {
			if (i + k + 1 > length)
				continue;

			memcpy(temp, x, sizeof(long long)*(i + k + 1));
			m[i] = median(temp, i + k + 1);
		} else if (i >= length - k) {
			memcpy(temp, &x[i - k], sizeof(long long)*(length - i + k));
			m[i] = median(temp, length - i + k);
		} else {
			memcpy(temp, &x[i - k], sizeof(long long)*(2 * k + 1));
			m[i] = median(temp, 2 * k + 1);
		}
	}

	for (i = 0; i < length; i++) {
		if (i < k)
			s[i] = buildS(x, m, 0, i + k, i);
		else if (i >= length - k)
			s[i] = buildS(x, m, i - k, length - 1, i);
		else
			s[i] = buildS(x, m, i - k, i + k, i);

		s[i] *= kappa;
	}

	for (i = 0; i < length; i++) {
		if (x[i] - m[i] > nsig*s[i] || x[i] - m[i] < -(nsig*s[i]))
			x[i] = m[i];
	}

	kfree(m);
	kfree(s);
	kfree(temp);
}

long long mean(long long* arr, int length)
{
	long long ret = arr[0];
	int n;

	for (n = 1; n < length; n++)
		ret = (n*ret + arr[n]) / (n + 1);

	return ret;
}

int calibration_trx_data(struct wacom_i2c *wac_i2c)
{
	struct wacom_elec_data *edata = wac_i2c->pdata->edata;
	long long *cal_xx_raw, *cal_xy_raw, *cal_yx_raw, *cal_yy_raw;
	long long cal_xx, cal_xy, cal_yx, cal_yy;
	int i;

	cal_xx_raw = cal_xy_raw = cal_yx_raw = cal_yy_raw = NULL;
	cal_xx = cal_xy = cal_yx = cal_yy = 0;

	cal_xx_raw = kzalloc(edata->max_x_ch * sizeof(long long), GFP_KERNEL);
	cal_xy_raw = kzalloc(edata->max_x_ch * sizeof(long long), GFP_KERNEL);
	cal_yx_raw = kzalloc(edata->max_y_ch * sizeof(long long), GFP_KERNEL);
	cal_yy_raw = kzalloc(edata->max_y_ch * sizeof(long long), GFP_KERNEL);
	if (!cal_xx_raw || !cal_xy_raw || !cal_yx_raw || !cal_yy_raw) {
		if (cal_xx_raw)
			kfree(cal_xx_raw);
		if (cal_xy_raw)
			kfree(cal_xy_raw);
		if (cal_yx_raw)
			kfree(cal_yx_raw);
		if (cal_yy_raw)
			kfree(cal_yy_raw);

		return -ENOMEM;
	}

	for (i = 0; i < edata->max_x_ch; i++) {
		cal_xx_raw[i] = edata->xx_ref[i] * POWER_OFFSET / edata->xx[i];
		cal_xy_raw[i] = edata->xy_ref[i] * POWER_OFFSET / edata->xy[i];
	}

	for (i = 0; i < edata->max_y_ch; i++) {
		cal_yx_raw[i] = edata->yx_ref[i] * POWER_OFFSET / edata->yx[i];
		cal_yy_raw[i] = edata->yx_ref[i] * POWER_OFFSET / edata->yx[i];
	}

	hampel(cal_xx_raw, edata->max_x_ch, 3, 3);
	hampel(cal_xy_raw, edata->max_x_ch, 3, 3);
	hampel(cal_yx_raw, edata->max_y_ch, 3, 3);
	hampel(cal_yy_raw, edata->max_y_ch, 3, 3);

	cal_xx = mean(cal_xx_raw, edata->max_x_ch);
	cal_xy = mean(cal_xy_raw, edata->max_x_ch);
	cal_yx = mean(cal_yx_raw, edata->max_y_ch);
	cal_yy = mean(cal_yy_raw, edata->max_y_ch);

	for (i = 0; i < edata->max_x_ch; i++) {
		edata->xx_xx[i] = cal_xx * edata->xx[i];
		edata->xy_xy[i] = cal_xy * edata->xy[i];
	}

	for (i = 0; i < edata->max_y_ch; i++) {
		edata->yx_yx[i] = cal_yx * edata->yx[i];
		edata->yy_yy[i] = cal_yy * edata->yy[i];
	}

	kfree(cal_xx_raw);
	kfree(cal_xy_raw);
	kfree(cal_yx_raw);
	kfree(cal_yy_raw);

	return 0;
}
/*
int cal_xy(struct wacom_i2c *wac_i2c)
{
	struct wacom_elec_data *edata = wac_i2c->pdata->edata;
	long long *cal_xy_raw = NULL;
	long long cal_xy = 0;
	int i;

	cal_xy_raw = kzalloc(edata->max_x_ch * sizeof(long long), GFP_KERNEL);
	if (!cal_xy_raw)
		return -ENOMEM;

	for (i = 0; i < edata->max_x_ch; i++)
		cal_xy_raw[i] = edata->xy_ref[i] * POWER_OFFSET / edata->xy[i];

	hampel(cal_xy_raw, edata->max_x_ch, 3, 3);

	cal_xy = mean(cal_xy_raw, edata->max_x_ch);

	for (i = 0; i < edata->max_x_ch; i++)
		edata->xy_xy[i] = cal_xy * edata->xy[i];

	kfree(cal_xy_raw);

	return 0;
}

int cal_yx(struct wacom_i2c *wac_i2c)
{
	struct wacom_elec_data *edata = wac_i2c->pdata->edata;
	long long *cal_yx_raw = NULL;
	long long cal_yx = 0;
	int i;

	cal_yx_raw = kzalloc(edata->max_y_ch * sizeof(long long), GFP_KERNEL);
	if (!cal_yx_raw)
		return -ENOMEM;

	for (i = 0; i < edata->max_y_ch; i++)
		cal_yx_raw[i] = edata->yx_ref[i] * POWER_OFFSET / edata->yx[i];

	hampel(cal_yx_raw, edata->max_y_ch, 3, 3);

	cal_yx = mean(cal_yx_raw, edata->max_y_ch);

	for (i = 0; i < edata->max_y_ch; i++)
		edata->yx_yx[i] = cal_yx * edata->yx[i];

	kfree(cal_yx_raw);

	return 0;
}

int cal_yy(struct wacom_i2c *wac_i2c)
{
	struct wacom_elec_data *edata = wac_i2c->pdata->edata;
	long long *cal_yy_raw = NULL;
	long long cal_yy = 0;
	int i;

	cal_yy_raw = kzalloc(edata->max_y_ch * sizeof(long long), GFP_KERNEL);
	if (!cal_yy_raw)
		return -ENOMEM;

	for (i = 0; i < edata->max_y_ch; i++)
		cal_yy_raw[i] = edata->yy_ref[i] * POWER_OFFSET / edata->yy[i];

	hampel(cal_yy_raw, edata->max_y_ch, 3, 3);

	cal_yy = mean(cal_yy_raw, edata->max_y_ch);

	for (i = 0; i < edata->max_y_ch; i++)
		edata->yy_yy[i] = cal_yy * edata->yy[i];

	kfree(cal_yy_raw);

	return 0;
}
*/
void calculate_ratio(struct wacom_i2c *wac_i2c)
{
	struct wacom_elec_data *edata = wac_i2c->pdata->edata;
	int i;

	edata->rxx[0] = edata->xx[1] * POWER_OFFSET / edata->xx[0];
	for (i = 1; i < edata->max_x_ch; i++)
		edata->rxx[i] = edata->xx[i - 1] * POWER_OFFSET / edata->xx[i];

	edata->rxy[0] = edata->xy[1] * POWER_OFFSET / edata->xy[0];
	for (i = 1; i < edata->max_x_ch; i++)
		edata->rxy[i] = edata->xy[i - 1] * POWER_OFFSET / edata->xy[i];

	edata->ryx[0] = edata->yx[1] * POWER_OFFSET / edata->yx[0];
	for (i = 1; i < edata->max_y_ch; i++)
		edata->ryx[i] = edata->yx[i - 1] * POWER_OFFSET / edata->yx[i];

	edata->ryy[0] = edata->yy[1] * POWER_OFFSET / edata->yy[0];
	for (i = 1; i < edata->max_y_ch; i++)
		edata->ryy[i] = edata->yy[i - 1] * POWER_OFFSET / edata->yy[i];
}

void make_decision(struct wacom_i2c *wac_i2c, u16* arrResult)
{
	struct wacom_elec_data *edata = wac_i2c->pdata->edata;
	u32 open_count, short_count;
	int i;

	open_count = short_count = 0;
	for (i = 0; i < edata->max_x_ch; i++) {
		edata->drxx[i] = edata->rxx[i] - edata->rxx_ref[i];
		edata->drxy[i] = edata->rxy[i] - edata->rxy_ref[i];

		if (edata->xy[i] < edata->xy_ref[i] / 2 || edata->xx[i] < edata->xx_ref[i] / 2) {
			arrResult[i + 1] |= SEC_OPEN;
			open_count++;
		}

		if (edata->xy_xy[i] > edata->xy_spec[i] || edata->xx_xx[i] > edata->xx_spec[i])
			arrResult[i + 1] |= SEC_SHORT;

		if (edata->drxy[i] > edata->drxy_spec[i] || edata->drxy[i] < -edata->drxy_spec[i])
			arrResult[i + 1] |= SEC_SHORT;

		if (edata->drxx[i] > edata->drxx_spec[i] || edata->drxx[i] < -edata->drxx_spec[i])
			arrResult[i + 1] |= SEC_SHORT;

		if (arrResult[i + 1] & SEC_SHORT)
			short_count++;
	}

	for (i = 0; i < edata->max_y_ch; i++) {
		edata->dryy[i] = edata->ryy[i] - edata->ryy_ref[i];
		edata->dryx[i] = edata->ryx[i] - edata->ryx_ref[i];

		if (edata->yx[i] < edata->yx_ref[i] / 2 || edata->yy[i] < edata->yy_ref[i] / 2) {
			arrResult[i + 1 + edata->max_x_ch] |= SEC_OPEN;
			open_count++;
		}

		if (edata->yx_yx[i] > edata->yx_spec[i] || edata->yy_yy[i] > edata->yy_spec[i])
			arrResult[i + 1 + edata->max_x_ch] |= SEC_SHORT;

		if (edata->dryx[i] > edata->dryx_spec[i] || edata->dryx[i] < -edata->dryx_spec[i])
			arrResult[i + 1 + edata->max_x_ch] |= SEC_SHORT;

		if (edata->dryy[i] > edata->dryy_spec[i] || edata->dryy[i] < -edata->dryy_spec[i])
			arrResult[i + 1 + edata->max_x_ch] |= SEC_SHORT;

		if (arrResult[i + 1 + edata->max_x_ch] & SEC_SHORT)
			short_count++;
	}

	arrResult[0] = (short_count << 8) + open_count;
}

void print_elec_data(struct wacom_i2c *wac_i2c)
{
	struct wacom_elec_data *edata = wac_i2c->pdata->edata;
	u8 *pstr = NULL;
	u8 ptmp[CMD_RESULT_WORD_LEN] = { 0 };
	int chsize, lsize;
	int i, j;

	input_info(true, &wac_i2c->client->dev, "%s\n", __func__);

	chsize = edata->max_x_ch + edata->max_y_ch;
	lsize = CMD_RESULT_WORD_LEN * (chsize + 1);

	pstr = kzalloc(lsize, GFP_KERNEL);
	if (pstr == NULL)
		return;

	memset(pstr, 0x0, lsize);
	snprintf(ptmp, sizeof(ptmp), "      TX");
	strlcat(pstr, ptmp, lsize);

	for (i = 0; i < chsize; i++) {
		snprintf(ptmp, sizeof(ptmp), " %02d ", i);
		strlcat(pstr, ptmp, lsize);
	}

	input_info(true, &wac_i2c->client->dev, "%s\n", pstr);
	memset(pstr, 0x0, lsize);
	snprintf(ptmp, sizeof(ptmp), " +");
	strlcat(pstr, ptmp, lsize);

	for (i = 0; i < chsize; i++) {
		snprintf(ptmp, sizeof(ptmp), "----");
		strlcat(pstr, ptmp, lsize);
	}

	input_info(true, &wac_i2c->client->dev, "%s\n", pstr);

	for (i = 0; i < chsize; i++) {
		memset(pstr, 0x0, lsize);
		snprintf(ptmp, sizeof(ptmp), "Rx%02d | ", i);
		strlcat(pstr, ptmp, lsize);

		for (j = 0; j < chsize; j++) {
			snprintf(ptmp, sizeof(ptmp), " %4d",
				 edata->elec_data[(i * chsize) + j]);

			strlcat(pstr, ptmp, lsize);
		}
		input_info(true, &wac_i2c->client->dev, "%s\n", pstr);
	}
	kfree(pstr);
}

void print_trx_data(struct wacom_i2c *wac_i2c)
{
	struct wacom_elec_data *edata = wac_i2c->pdata->edata;
	struct i2c_client *client = wac_i2c->client;
	u8 tmp_buf[CMD_RESULT_WORD_LEN] = { 0 };
	u8 *buff;
	int buff_size;
	int i;

	buff_size = edata->max_x_ch > edata->max_y_ch ? edata->max_x_ch : edata->max_y_ch;
	buff_size = CMD_RESULT_WORD_LEN * (buff_size + 1);

	buff = kzalloc(buff_size, GFP_KERNEL);
	if (buff == NULL)
		return;

	snprintf(tmp_buf, CMD_RESULT_WORD_LEN, "xx: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_x_ch; i++) {
		snprintf(tmp_buf, CMD_RESULT_WORD_LEN, "%d ", edata->xx[i]);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, CMD_RESULT_WORD_LEN);
	}

	input_info(true, &client->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, CMD_RESULT_WORD_LEN, "xy: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_x_ch; i++) {
		snprintf(tmp_buf, CMD_RESULT_WORD_LEN, "%d ", edata->xy[i]);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, CMD_RESULT_WORD_LEN);
	}

	input_info(true, &client->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, CMD_RESULT_WORD_LEN, "yx: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_y_ch; i++) {
		snprintf(tmp_buf, CMD_RESULT_WORD_LEN, "%d ", edata->yx[i]);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, CMD_RESULT_WORD_LEN);
	}

	input_info(true, &client->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, CMD_RESULT_WORD_LEN, "yy: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_y_ch; i++) {
		snprintf(tmp_buf, CMD_RESULT_WORD_LEN, "%d ", edata->yy[i]);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, CMD_RESULT_WORD_LEN);
	}

	input_info(true, &client->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	kfree(buff);
}

void print_cal_trx_data(struct wacom_i2c *wac_i2c)
{
	struct wacom_elec_data *edata = wac_i2c->pdata->edata;
	struct i2c_client *client = wac_i2c->client;
	u8 tmp_buf[CMD_RESULT_WORD_LEN] = { 0 };
	u8 *buff;
	int buff_size;
	int i;

	buff_size = edata->max_x_ch > edata->max_y_ch ? edata->max_x_ch : edata->max_y_ch;
	buff_size = CMD_RESULT_WORD_LEN * (buff_size + 1);

	buff = kzalloc(buff_size, GFP_KERNEL);
	if (buff == NULL)
		return;

	snprintf(tmp_buf, CMD_RESULT_WORD_LEN, "xx_xx: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_x_ch; i++) {
		snprintf(tmp_buf, CMD_RESULT_WORD_LEN, "%ld ", edata->xx_xx[i]);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, CMD_RESULT_WORD_LEN);
	}

	input_info(true, &client->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, CMD_RESULT_WORD_LEN, "xy_xy: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_x_ch; i++) {
		snprintf(tmp_buf, CMD_RESULT_WORD_LEN, "%ld ", edata->xy_xy[i]);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, CMD_RESULT_WORD_LEN);
	}

	input_info(true, &client->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, CMD_RESULT_WORD_LEN, "yx_yx: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_y_ch; i++) {
		snprintf(tmp_buf, CMD_RESULT_WORD_LEN, "%ld ", edata->yx_yx[i]);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, CMD_RESULT_WORD_LEN);
	}

	input_info(true, &client->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, CMD_RESULT_WORD_LEN, "yy_yy: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_y_ch; i++) {
		snprintf(tmp_buf, CMD_RESULT_WORD_LEN, "%ld ", edata->yy_yy[i]);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, CMD_RESULT_WORD_LEN);
	}

	input_info(true, &client->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	kfree(buff);
}

void print_ratio_trx_data(struct wacom_i2c *wac_i2c)
{
	struct wacom_elec_data *edata = wac_i2c->pdata->edata;
	struct i2c_client *client = wac_i2c->client;
	u8 tmp_buf[CMD_RESULT_WORD_LEN] = { 0 };
	u8 *buff;
	int buff_size;
	int i;

	buff_size = edata->max_x_ch > edata->max_y_ch ? edata->max_x_ch : edata->max_y_ch;
	buff_size = CMD_RESULT_WORD_LEN * (buff_size + 1);

	buff = kzalloc(buff_size, GFP_KERNEL);
	if (buff == NULL)
		return;

	snprintf(tmp_buf, CMD_RESULT_WORD_LEN, "rxx: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_x_ch; i++) {
		snprintf(tmp_buf, CMD_RESULT_WORD_LEN, "%ld ", edata->rxx[i]);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, CMD_RESULT_WORD_LEN);
	}

	input_info(true, &client->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, CMD_RESULT_WORD_LEN, "rxy: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_x_ch; i++) {
		snprintf(tmp_buf, CMD_RESULT_WORD_LEN, "%ld ", edata->rxy[i]);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, CMD_RESULT_WORD_LEN);
	}

	input_info(true, &client->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, CMD_RESULT_WORD_LEN, "ryx: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_y_ch; i++) {
		snprintf(tmp_buf, CMD_RESULT_WORD_LEN, "%ld ", edata->ryx[i]);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, CMD_RESULT_WORD_LEN);
	}

	input_info(true, &client->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, CMD_RESULT_WORD_LEN, "ryy: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_y_ch; i++) {
		snprintf(tmp_buf, CMD_RESULT_WORD_LEN, "%ld ", edata->ryy[i]);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, CMD_RESULT_WORD_LEN);
	}

	input_info(true, &client->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	kfree(buff);
}

void print_difference_ratio_trx_data(struct wacom_i2c *wac_i2c)
{
	struct wacom_elec_data *edata = wac_i2c->pdata->edata;
	struct i2c_client *client = wac_i2c->client;
	u8 tmp_buf[CMD_RESULT_WORD_LEN] = { 0 };
	u8 *buff;
	int buff_size;
	int i;

	buff_size = edata->max_x_ch > edata->max_y_ch ? edata->max_x_ch : edata->max_y_ch;
	buff_size = CMD_RESULT_WORD_LEN * (buff_size + 1);

	buff = kzalloc(buff_size, GFP_KERNEL);
	if (buff == NULL)
		return;

	snprintf(tmp_buf, CMD_RESULT_WORD_LEN, "drxx: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_x_ch; i++) {
		snprintf(tmp_buf, CMD_RESULT_WORD_LEN, "%ld ", edata->drxx[i]);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, CMD_RESULT_WORD_LEN);
	}

	input_info(true, &client->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, CMD_RESULT_WORD_LEN, "drxy: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_x_ch; i++) {
		snprintf(tmp_buf, CMD_RESULT_WORD_LEN, "%ld ", edata->drxy[i]);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, CMD_RESULT_WORD_LEN);
	}

	input_info(true, &client->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, CMD_RESULT_WORD_LEN, "dryx: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_y_ch; i++) {
		snprintf(tmp_buf, CMD_RESULT_WORD_LEN, "%ld ", edata->dryx[i]);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, CMD_RESULT_WORD_LEN);
	}

	input_info(true, &client->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, CMD_RESULT_WORD_LEN, "dryy: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_y_ch; i++) {
		snprintf(tmp_buf, CMD_RESULT_WORD_LEN, "%ld ", edata->dryy[i]);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, CMD_RESULT_WORD_LEN);
	}

	input_info(true, &client->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	kfree(buff);
}
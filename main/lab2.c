#include <string.h>
#include "esp_log.h"
#include "driver/i2c.h"
#include "sdkconfig.h"
#include "ssd1306.h"
#include "UIT_logo.h"

static const char *TAG = "OLED I2C";

#define _I2C_NUMBER(num)            I2C_NUM_##num
#define I2C_NUMBER(num)             _I2C_NUMBER(num)

#define I2C_MASTER_SCL_IO           CONFIG_I2C_MASTER_SCL                   /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO           CONFIG_I2C_MASTER_SDA                   /*!< gpio number for I2C master data  */
#define I2C_MASTER_NUM              I2C_NUMBER(CONFIG_I2C_MASTER_PORT_NUM)  /*!< I2C port number for master dev */
#define I2C_MASTER_FREQ_HZ          100000U                                 /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE   0                                       /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0                                       /*!< I2C master doesn't need buffer */

static esp_err_t i2c_master_init(void)
{
    int i2c_master_port = I2C_MASTER_NUM;
    i2c_config_t conf = {
        .mode               = I2C_MODE_MASTER,
        .sda_io_num         = I2C_MASTER_SDA_IO,
        .sda_pullup_en      = GPIO_PULLUP_ENABLE,
        .scl_io_num         = I2C_MASTER_SCL_IO,
        .scl_pullup_en      = GPIO_PULLUP_ENABLE,
        .master.clk_speed   = I2C_MASTER_FREQ_HZ,
        .clk_flags          = I2C_SCLK_SRC_FLAG_FOR_NOMAL,          
    };
    esp_err_t err = i2c_param_config(i2c_master_port, &conf);
    if (err != ESP_OK) 
    {
        return err;
    }
	ESP_LOGI(TAG, "I2C configured successfully");
    return i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

static esp_err_t ssd1306_init(void)
{
    esp_err_t espRc;

	i2c_cmd_handle_t cmd;
	cmd = i2c_cmd_link_create();

	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (OLED_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
	i2c_master_write_byte(cmd, OLED_CONTROL_BYTE_CMD_STREAM, true);

	i2c_master_write_byte(cmd, OLED_CMD_SET_CHARGE_PUMP, true);
	i2c_master_write_byte(cmd, 0x14, true);

	i2c_master_write_byte(cmd, OLED_CMD_SET_SEGMENT_REMAP, true); // reverse left-right mapping
	i2c_master_write_byte(cmd, OLED_CMD_SET_COM_SCAN_MODE, true); // reverse up-bottom mapping

	i2c_master_write_byte(cmd, OLED_CMD_DISPLAY_NORMAL, true);
    i2c_master_write_byte(cmd, OLED_CMD_DISPLAY_OFF, true);
	i2c_master_write_byte(cmd, OLED_CMD_DISPLAY_ON, true);
	i2c_master_stop(cmd);

	espRc = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 10/portTICK_PERIOD_MS);
	if (espRc == ESP_OK) 
	{
		ESP_LOGI(TAG, "OLED configured successfully");
	} 
	else 
	{
		ESP_LOGE(TAG, "OLED configuration failed. code: 0x%.2X", espRc);
	}
	i2c_cmd_link_delete(cmd);

    return espRc;
}

void task_ssd1306_display_text(void* ignore) 
{
	i2c_cmd_handle_t cmd;
	
	uint8_t cur_page = 0;

	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (OLED_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);

	i2c_master_write_byte(cmd, OLED_CONTROL_BYTE_CMD_STREAM, true);
	i2c_master_write_byte(cmd, 0x00, true); 
	i2c_master_write_byte(cmd, 0x10, true);
	i2c_master_write_byte(cmd, 0xB0 | cur_page, true);

	i2c_master_stop(cmd);
	i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 10 / portTICK_PERIOD_MS);
	i2c_cmd_link_delete(cmd);

	for (uint8_t i = 0; i < 8; i++) 
	{
		cmd = i2c_cmd_link_create();
		i2c_master_start(cmd);
		i2c_master_write_byte(cmd, (OLED_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);

		i2c_master_write_byte(cmd, OLED_CONTROL_BYTE_DATA_STREAM, true);
		i2c_master_write(cmd, UIT_page[i], 128, true);

		i2c_master_stop(cmd);
		i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 10 / portTICK_PERIOD_MS);
		i2c_cmd_link_delete(cmd);

		cmd = i2c_cmd_link_create();
		i2c_master_start(cmd);
		i2c_master_write_byte(cmd, (OLED_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);

		i2c_master_write_byte(cmd, OLED_CONTROL_BYTE_CMD_STREAM, true);
		i2c_master_write_byte(cmd, 0x00, true); 
		i2c_master_write_byte(cmd, 0x10, true);
		i2c_master_write_byte(cmd, 0xB0 | ++cur_page, true); 

		i2c_master_stop(cmd);
		i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 10 / portTICK_PERIOD_MS);
		i2c_cmd_link_delete(cmd);
	}

	vTaskDelete(NULL);
}

void task_ssd1306_display_clear(void *ignore) 
{
	i2c_cmd_handle_t cmd;

	uint8_t clear[128];
	for (uint8_t i = 0; i < 128; i++) 
	{
		clear[i] = 0;
	}
	for (uint8_t i = 0; i < 8; i++) 
	{
		cmd = i2c_cmd_link_create();
		i2c_master_start(cmd);
		i2c_master_write_byte(cmd, (OLED_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
		i2c_master_write_byte(cmd, OLED_CONTROL_BYTE_CMD_SINGLE, true);
		i2c_master_write_byte(cmd, 0xB0 | i, true);

		i2c_master_write_byte(cmd, OLED_CONTROL_BYTE_DATA_STREAM, true);
		i2c_master_write(cmd, clear, 128, true);
		i2c_master_stop(cmd);
		i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 10 / portTICK_PERIOD_MS);
		i2c_cmd_link_delete(cmd);
	}

	vTaskDelete(NULL);
}

void app_main(void)
{
    ESP_ERROR_CHECK(i2c_master_init());
	ssd1306_init();
	convert();
	xTaskCreate(task_ssd1306_display_text, "display_UITlogo", 1024 * 1, (void *)0, 10, NULL);
}

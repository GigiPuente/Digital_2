/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    user_diskio.c
  * @brief   This file includes a diskio driver skeleton to be completed by the user.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

#ifdef USE_OBSOLETE_USER_CODE_SECTION_0
/*
 * Warning: the user section 0 is no more in use (starting from CubeMx version 4.16.0)
 * To be suppressed in the future.
 * Kept to ensure backward compatibility with previous CubeMx versions when
 * migrating projects.
 * User code previously added there should be copied in the new user sections before
 * the section contents can be deleted.
 */
/* USER CODE BEGIN 0 */
/* USER CODE END 0 */
#endif

/* USER CODE BEGIN DECL */

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "ff_gen_drv.h"
#include "main.h"
#include <stdbool.h>

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define SD_CMD0             0U
#define SD_CMD1             1U
#define SD_CMD8             8U
#define SD_CMD9             9U
#define SD_CMD12           12U
#define SD_CMD16           16U
#define SD_CMD17           17U
#define SD_CMD18           18U
#define SD_CMD23           23U
#define SD_CMD24           24U
#define SD_CMD25           25U
#define SD_CMD55           55U
#define SD_CMD58           58U
#define SD_ACMD13         141U
#define SD_ACMD23         151U
#define SD_ACMD41         169U
#define SD_TOKEN_DATOS   0xFEU
#define SD_TOKEN_MULTI   0xFCU
#define SD_TOKEN_FIN     0xFDU
#define SD_TIPO_MMC      0x01U
#define SD_TIPO_SD1      0x02U
#define SD_TIPO_SD2      0x04U
#define SD_TIPO_BLOQUE   0x08U

/* Private variables ---------------------------------------------------------*/
/* Disk status */
static volatile DSTATUS Stat = STA_NOINIT;
static uint8_t sd_tipo = 0U;

extern SPI_HandleTypeDef hspi1;

static void SD_CS_Alto(void)
{
  HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_SET);
}

static void SD_CS_Bajo(void)
{
  HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_RESET);
}

static uint8_t SD_SPI_Byte(uint8_t dato)
{
  uint8_t recibido = 0xFFU;
  HAL_SPI_TransmitReceive(&hspi1, &dato, &recibido, 1U, 100U);
  return recibido;
}

static bool SD_SPI_Recibir(uint8_t *destino, UINT cantidad)
{
  uint8_t relleno[32];

  memset(relleno, 0xFF, sizeof(relleno));
  while (cantidad > 0U)
  {
    uint16_t bloque = (cantidad > sizeof(relleno)) ?
                      (uint16_t)sizeof(relleno) : (uint16_t)cantidad;

    if (HAL_SPI_TransmitReceive(&hspi1, relleno, destino,
                                bloque, 1000U) != HAL_OK)
    {
      return false;
    }

    destino += bloque;
    cantidad -= bloque;
  }

  return true;
}

static bool SD_SPI_Enviar(const uint8_t *origen, UINT cantidad)
{
  return HAL_SPI_Transmit(&hspi1, (uint8_t *)origen,
                          (uint16_t)cantidad, 1000U) == HAL_OK;
}

static void SD_SPI_Velocidad(uint32_t prescaler)
{
  __HAL_SPI_DISABLE(&hspi1);
  MODIFY_REG(hspi1.Instance->CR1, SPI_CR1_BR, prescaler);
  hspi1.Init.BaudRatePrescaler = prescaler;
  __HAL_SPI_ENABLE(&hspi1);
}

static bool SD_EsperarListo(uint32_t timeout)
{
  uint32_t inicio = HAL_GetTick();

  do
  {
    if (SD_SPI_Byte(0xFFU) == 0xFFU)
    {
      return true;
    }
  } while ((HAL_GetTick() - inicio) < timeout);

  return false;
}

static void SD_Deseleccionar(void)
{
  SD_CS_Alto();
  SD_SPI_Byte(0xFFU);
}

static bool SD_Seleccionar(void)
{
  SD_CS_Bajo();
  SD_SPI_Byte(0xFFU);

  if (SD_EsperarListo(500U))
  {
    return true;
  }

  SD_Deseleccionar();
  return false;
}

static uint8_t SD_EnviarComando(uint8_t comando, uint32_t argumento)
{
  uint8_t paquete[6];
  uint8_t respuesta;

  if ((comando & 0x80U) != 0U)
  {
    comando &= 0x7FU;
    respuesta = SD_EnviarComando(SD_CMD55, 0U);
    if (respuesta > 1U)
    {
      return respuesta;
    }
  }

  if (comando != SD_CMD12)
  {
    SD_Deseleccionar();
    if (!SD_Seleccionar())
    {
      return 0xFFU;
    }
  }

  paquete[0] = (uint8_t)(0x40U | comando);
  paquete[1] = (uint8_t)(argumento >> 24U);
  paquete[2] = (uint8_t)(argumento >> 16U);
  paquete[3] = (uint8_t)(argumento >> 8U);
  paquete[4] = (uint8_t)argumento;
  paquete[5] = 0x01U;

  if (comando == SD_CMD0)
  {
    paquete[5] = 0x95U;
  }
  else if (comando == SD_CMD8)
  {
    paquete[5] = 0x87U;
  }

  if (!SD_SPI_Enviar(paquete, sizeof(paquete)))
  {
    return 0xFFU;
  }

  if (comando == SD_CMD12)
  {
    SD_SPI_Byte(0xFFU);
  }

  for (uint8_t intento = 0U; intento < 10U; intento++)
  {
    respuesta = SD_SPI_Byte(0xFFU);
    if ((respuesta & 0x80U) == 0U)
    {
      return respuesta;
    }
  }

  return 0xFFU;
}

static bool SD_RecibirBloque(uint8_t *buffer, UINT cantidad)
{
  uint32_t inicio = HAL_GetTick();
  uint8_t token;

  do
  {
    token = SD_SPI_Byte(0xFFU);
  } while ((token == 0xFFU) && ((HAL_GetTick() - inicio) < 200U));

  if (token != SD_TOKEN_DATOS)
  {
    return false;
  }

  if (!SD_SPI_Recibir(buffer, cantidad))
  {
    return false;
  }

  SD_SPI_Byte(0xFFU);
  SD_SPI_Byte(0xFFU);
  return true;
}

#if _USE_WRITE == 1
static bool SD_EnviarBloque(const uint8_t *buffer, uint8_t token)
{
  uint8_t respuesta;

  if (!SD_EsperarListo(500U))
  {
    return false;
  }

  SD_SPI_Byte(token);
  if (token == SD_TOKEN_FIN)
  {
    return true;
  }

  if (!SD_SPI_Enviar(buffer, 512U))
  {
    return false;
  }

  SD_SPI_Byte(0xFFU);
  SD_SPI_Byte(0xFFU);
  respuesta = SD_SPI_Byte(0xFFU);
  if ((respuesta & 0x1FU) != 0x05U)
  {
    return false;
  }

  return SD_EsperarListo(500U);
}
#endif

/* USER CODE END DECL */

/* Private function prototypes -----------------------------------------------*/
DSTATUS USER_initialize (BYTE pdrv);
DSTATUS USER_status (BYTE pdrv);
DRESULT USER_read (BYTE pdrv, BYTE *buff, DWORD sector, UINT count);
#if _USE_WRITE == 1
  DRESULT USER_write (BYTE pdrv, const BYTE *buff, DWORD sector, UINT count);
#endif /* _USE_WRITE == 1 */
#if _USE_IOCTL == 1
  DRESULT USER_ioctl (BYTE pdrv, BYTE cmd, void *buff);
#endif /* _USE_IOCTL == 1 */

Diskio_drvTypeDef  USER_Driver =
{
  USER_initialize,
  USER_status,
  USER_read,
#if  _USE_WRITE
  USER_write,
#endif  /* _USE_WRITE == 1 */
#if  _USE_IOCTL == 1
  USER_ioctl,
#endif /* _USE_IOCTL == 1 */
};

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Initializes a Drive
  * @param  pdrv: Physical drive number (0..)
  * @retval DSTATUS: Operation status
  */
DSTATUS USER_initialize (
	BYTE pdrv           /* Physical drive nmuber to identify the drive */
)
{
  /* USER CODE BEGIN INIT */
  uint8_t tipo = 0U;
  uint8_t respuesta;
  uint8_t ocr[4];
  uint32_t inicio;

  if (pdrv != 0U)
  {
    return STA_NOINIT;
  }

  Stat = STA_NOINIT;
  sd_tipo = 0U;
  SD_SPI_Velocidad(SPI_BAUDRATEPRESCALER_256);
  SD_CS_Alto();
  HAL_Delay(10U);
  for (uint8_t i = 0U; i < 10U; i++)
  {
    SD_SPI_Byte(0xFFU);
  }

  inicio = HAL_GetTick();
  do
  {
    respuesta = SD_EnviarComando(SD_CMD0, 0U);
  } while ((respuesta != 1U) && ((HAL_GetTick() - inicio) < 1000U));

  if (respuesta == 1U)
  {
    if (SD_EnviarComando(SD_CMD8, 0x1AAU) == 1U)
    {
      for (uint8_t i = 0U; i < 4U; i++)
      {
        ocr[i] = SD_SPI_Byte(0xFFU);
      }

      if ((ocr[2] == 0x01U) && (ocr[3] == 0xAAU))
      {
        inicio = HAL_GetTick();
        do
        {
          respuesta = SD_EnviarComando(SD_ACMD41, 1UL << 30U);
        } while ((respuesta != 0U) &&
                 ((HAL_GetTick() - inicio) < 1500U));

        if ((respuesta == 0U) &&
            (SD_EnviarComando(SD_CMD58, 0U) == 0U))
        {
          for (uint8_t i = 0U; i < 4U; i++)
          {
            ocr[i] = SD_SPI_Byte(0xFFU);
          }
          tipo = (uint8_t)(SD_TIPO_SD2 |
                 ((ocr[0] & 0x40U) ? SD_TIPO_BLOQUE : 0U));
        }
      }
    }
    else
    {
      uint8_t comando;

      if (SD_EnviarComando(SD_ACMD41, 0U) <= 1U)
      {
        tipo = SD_TIPO_SD1;
        comando = SD_ACMD41;
      }
      else
      {
        tipo = SD_TIPO_MMC;
        comando = SD_CMD1;
      }

      inicio = HAL_GetTick();
      do
      {
        respuesta = SD_EnviarComando(comando, 0U);
      } while ((respuesta != 0U) &&
               ((HAL_GetTick() - inicio) < 1500U));

      if ((respuesta != 0U) ||
          (SD_EnviarComando(SD_CMD16, 512U) != 0U))
      {
        tipo = 0U;
      }
    }
  }

  sd_tipo = tipo;
  SD_Deseleccionar();

  if (tipo != 0U)
  {
    Stat &= (DSTATUS)~STA_NOINIT;
    SD_SPI_Velocidad(SPI_BAUDRATEPRESCALER_4);
  }

  return Stat;
  /* USER CODE END INIT */
}

/**
  * @brief  Gets Disk Status
  * @param  pdrv: Physical drive number (0..)
  * @retval DSTATUS: Operation status
  */
DSTATUS USER_status (
	BYTE pdrv       /* Physical drive number to identify the drive */
)
{
  /* USER CODE BEGIN STATUS */
  if (pdrv != 0U)
  {
    return STA_NOINIT;
  }

  return Stat;
  /* USER CODE END STATUS */
}

/**
  * @brief  Reads Sector(s)
  * @param  pdrv: Physical drive number (0..)
  * @param  *buff: Data buffer to store read data
  * @param  sector: Sector address (LBA)
  * @param  count: Number of sectors to read (1..128)
  * @retval DRESULT: Operation result
  */
DRESULT USER_read (
	BYTE pdrv,      /* Physical drive nmuber to identify the drive */
	BYTE *buff,     /* Data buffer to store read data */
	DWORD sector,   /* Sector address in LBA */
	UINT count      /* Number of sectors to read */
)
{
  /* USER CODE BEGIN READ */
  if ((pdrv != 0U) || (buff == NULL) || (count == 0U))
  {
    return RES_PARERR;
  }

  if ((Stat & STA_NOINIT) != 0U)
  {
    return RES_NOTRDY;
  }

  if ((sd_tipo & SD_TIPO_BLOQUE) == 0U)
  {
    sector *= 512U;
  }

  if (count == 1U)
  {
    if ((SD_EnviarComando(SD_CMD17, sector) == 0U) &&
        SD_RecibirBloque(buff, 512U))
    {
      count = 0U;
    }
  }
  else if (SD_EnviarComando(SD_CMD18, sector) == 0U)
  {
    do
    {
      if (!SD_RecibirBloque(buff, 512U))
      {
        break;
      }
      buff += 512U;
    } while (--count != 0U);

    SD_EnviarComando(SD_CMD12, 0U);
  }

  SD_Deseleccionar();
  return (count == 0U) ? RES_OK : RES_ERROR;
  /* USER CODE END READ */
}

/**
  * @brief  Writes Sector(s)
  * @param  pdrv: Physical drive number (0..)
  * @param  *buff: Data to be written
  * @param  sector: Sector address (LBA)
  * @param  count: Number of sectors to write (1..128)
  * @retval DRESULT: Operation result
  */
#if _USE_WRITE == 1
DRESULT USER_write (
	BYTE pdrv,          /* Physical drive nmuber to identify the drive */
	const BYTE *buff,   /* Data to be written */
	DWORD sector,       /* Sector address in LBA */
	UINT count          /* Number of sectors to write */
)
{
  /* USER CODE BEGIN WRITE */
  /* USER CODE HERE */
  if ((pdrv != 0U) || (buff == NULL) || (count == 0U))
  {
    return RES_PARERR;
  }

  if ((Stat & STA_NOINIT) != 0U)
  {
    return RES_NOTRDY;
  }

  if ((sd_tipo & SD_TIPO_BLOQUE) == 0U)
  {
    sector *= 512U;
  }

  if (count == 1U)
  {
    if ((SD_EnviarComando(SD_CMD24, sector) == 0U) &&
        SD_EnviarBloque(buff, SD_TOKEN_DATOS))
    {
      count = 0U;
    }
  }
  else
  {
    UINT restantes = count;

    if ((sd_tipo & (SD_TIPO_SD1 | SD_TIPO_SD2)) != 0U)
    {
      SD_EnviarComando(SD_ACMD23, count);
    }

    if (SD_EnviarComando(SD_CMD25, sector) == 0U)
    {
      do
      {
        if (!SD_EnviarBloque(buff, SD_TOKEN_MULTI))
        {
          break;
        }
        buff += 512U;
      } while (--restantes != 0U);

      if (!SD_EnviarBloque(NULL, SD_TOKEN_FIN))
      {
        restantes = 1U;
      }
    }

    count = restantes;
  }

  SD_Deseleccionar();
  return (count == 0U) ? RES_OK : RES_ERROR;
  /* USER CODE END WRITE */
}
#endif /* _USE_WRITE == 1 */

/**
  * @brief  I/O control operation
  * @param  pdrv: Physical drive number (0..)
  * @param  cmd: Control code
  * @param  *buff: Buffer to send/receive control data
  * @retval DRESULT: Operation result
  */
#if _USE_IOCTL == 1
DRESULT USER_ioctl (
	BYTE pdrv,      /* Physical drive nmuber (0..) */
	BYTE cmd,       /* Control code */
	void *buff      /* Buffer to send/receive control data */
)
{
  /* USER CODE BEGIN IOCTL */
  DRESULT res = RES_ERROR;
  uint8_t csd[16];

  if (pdrv != 0U)
  {
    return RES_PARERR;
  }

  if ((Stat & STA_NOINIT) != 0U)
  {
    return RES_NOTRDY;
  }

  if (cmd == CTRL_SYNC)
  {
    if (SD_Seleccionar())
    {
      res = RES_OK;
    }
  }
  else if (cmd == GET_SECTOR_SIZE)
  {
    if (buff != NULL)
    {
      *(WORD *)buff = 512U;
      res = RES_OK;
    }
    else
    {
      res = RES_PARERR;
    }
  }
  else if (cmd == GET_BLOCK_SIZE)
  {
    if (buff != NULL)
    {
      *(DWORD *)buff = 1U;
      res = RES_OK;
    }
    else
    {
      res = RES_PARERR;
    }
  }
  else if (cmd == GET_SECTOR_COUNT)
  {
    if (buff == NULL)
    {
      res = RES_PARERR;
    }
    else if ((SD_EnviarComando(SD_CMD9, 0U) == 0U) &&
             SD_RecibirBloque(csd, 16U))
    {
      if ((csd[0] >> 6U) == 1U)
      {
        uint32_t tamano = (uint32_t)csd[9] |
                          ((uint32_t)csd[8] << 8U) |
                          ((uint32_t)(csd[7] & 0x3FU) << 16U);
        *(DWORD *)buff = (tamano + 1U) << 10U;
      }
      else
      {
        uint8_t desplazamiento = (uint8_t)((csd[5] & 0x0FU) +
                                  ((csd[10] & 0x80U) >> 7U) +
                                  ((csd[9] & 0x03U) << 1U) + 2U);
        uint32_t tamano = (uint32_t)(csd[8] >> 6U) |
                          ((uint32_t)csd[7] << 2U) |
                          ((uint32_t)(csd[6] & 0x03U) << 10U);
        *(DWORD *)buff = (tamano + 1U) << (desplazamiento - 9U);
      }
      res = RES_OK;
    }
  }
  else
  {
    res = RES_PARERR;
  }

  SD_Deseleccionar();
  return res;
  /* USER CODE END IOCTL */
}
#endif /* _USE_IOCTL == 1 */

#pragma once

#include <cstdint>

/**
 * @brief Менеджер epoll
 * 
 * Отвечает за:
 * - Создание и управление epoll-инстансом
 * - Добавление/удаление файловых дескрипторов
 * - Ожидание событий
 */
class EpollManager
{
public:
    /**
     * @brief Конструктор
     */
    EpollManager();

    /**
     * @brief Деструктор
     */
    ~EpollManager();

    /**
     * @brief Запрет копирования
     */
    EpollManager(const EpollManager&) = delete;
    EpollManager& operator=(const EpollManager&) = delete;

    /**
     * @brief Разрешение перемещения
     */
    EpollManager(EpollManager&&) noexcept;
    EpollManager& operator=(EpollManager&&) noexcept;

    /**
     * @brief Инициализация epoll-инстанса
     * @return true при успешной инициализации
     */
    [[nodiscard]] bool initialize();

    /**
     * @brief Добавление файлового дескриптора в epoll
     * @param fd Файловый дескриптор
     * @param events Маска событий
     * @return true при успехе
     */
    [[nodiscard]] bool addFileDescriptor(int fd, uint32_t events) const;

    /**
     * @brief Удаление файлового дескриптора из epoll
     * @param fd Файловый дескриптор
     * @return true при успехе
     */
    [[nodiscard]] bool removeFileDescriptor(int fd) const;

    /**
     * @brief Ожидание событий
     * @param events Массив для событий
     * @param max_events Максимальное количество событий
     * @param timeout Таймаут в миллисекундах (-1 для бесконечного ожидания)
     * @return Количество произошедших событий или -1 при ошибке
     */
    [[nodiscard]] int waitForEvents(struct epoll_event* events, int max_events, int timeout = -1) const;

    /**
     * @brief Проверка валидности epoll-инстанса
     * @return true если epoll инициализирован
     */
    [[nodiscard]] bool isValid() const noexcept { return epoll_fd_ >= 0; }

    /**
     * @brief Получение файлового дескриптора epoll
     * @return Файловый дескриптор epoll или -1
     */
    [[nodiscard]] int getEpollFd() const noexcept { return epoll_fd_; }

private:
    int epoll_fd_ = -1; ///< Файловый дескриптор epoll-инстанса
};

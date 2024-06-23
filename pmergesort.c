// tast3 멀티스레드 방식 -> 데이터 병렬로 정렬 작업
// 전체적으로 큰 데이터 배열 -> 여러 작은 부분으로 나눔, 각 부분을 스레드에서 정렬 후 병합하여 정렬 완성

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#include <unistd.h>

#define MAX_TASKS 200

double *data;
int n_data;
int max_tasks = MAX_TASKS;

enum task_status
{
    UNDONE,
    PROCESS,
    DONE
};

struct sorting_task
{
    double *a;
    int n_a;
    int status;
};

struct sorting_task *tasks;
int n_tasks = 0;
int n_undone = 0;
int n_done = 0;

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cv = PTHREAD_COND_INITIALIZER;

void merge_lists(double *a1, int n_a1, double *a2, int n_a2);
void merge_sort(double *a, int n_a);

void *worker(void *ptr)
{
    while (1)
    {
        pthread_mutex_lock(&m);
        while (n_undone == 0 && n_done < n_tasks)
        {
            pthread_cond_wait(&cv, &m);
        }

        if (n_done >= n_tasks)
        {
            pthread_mutex_unlock(&m);
            break;
        }

        int i;
        for (i = 0; i < n_tasks; i++)
        {
            if (tasks[i].status == UNDONE)
                break;
        }

        if (i == n_tasks)
        {
            pthread_mutex_unlock(&m);
            continue;
        }

        tasks[i].status = PROCESS;
        n_undone--;
        pthread_mutex_unlock(&m);

        printf("[Thread %ld] starts Task %d\n", pthread_self(), i);

        merge_sort(tasks[i].a, tasks[i].n_a);

        printf("[Thread %ld] completed Task %d\n", pthread_self(), i);

        pthread_mutex_lock(&m);
        tasks[i].status = DONE;
        n_done++;
        pthread_cond_broadcast(&cv);
        pthread_mutex_unlock(&m);
    }

    return NULL;
}

void *init_data(void *arg)
{
    int index = *(int *)arg;
    int start = index * (n_data / max_tasks);
    int end = (index + 1) * (n_data / max_tasks);
    if (index == max_tasks - 1)
    {
        end += n_data % max_tasks;
    }

    for (int i = start; i < end; i++)
    {
        int num = rand();   //숫자 랜덤으로 정렬
        int den = rand();
        if (den != 0)
            data[i] = ((double)num) / ((double)den);
        else
            data[i] = ((double)num);
    }
    return NULL;
}

void *merge_worker(void *arg)
{
    int task_id = *(int *)arg;

    while (1)
    {
        pthread_mutex_lock(&m);
        if (task_id >= n_tasks - 1 || tasks[task_id].status != DONE || tasks[task_id + 1].status != DONE)
        {
            pthread_mutex_unlock(&m);
            break;
        }

        merge_lists(tasks[task_id].a, tasks[task_id].n_a, tasks[task_id + 1].a, tasks[task_id + 1].n_a);
        tasks[task_id].n_a += tasks[task_id + 1].n_a;
        tasks[task_id + 1].status = UNDONE;
        pthread_mutex_unlock(&m);

        task_id += 2;
    }

    return NULL;
}


int main(int argc, char *argv[])
{
    struct timeval ts_start, ts_end;
    int opt, n_threads = 8;

    while ((opt = getopt(argc, argv, "d:t:")) != -1)
    {
        switch (opt)
        {
        case 'd':
            n_data = atoi(optarg);
            break;
        case 't':
            n_threads = atoi(optarg);
            break;
        default:
            fprintf(stderr, "Usage: %s -d <# data elements> -t <# threads>\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (n_data <= 0 || n_threads <= 0)
    {
        fprintf(stderr, "Invalid arguments\n");
        exit(EXIT_FAILURE);
    }

    data = (double *)malloc(n_data * sizeof(double));
    tasks = (struct sorting_task *)malloc(max_tasks * sizeof(struct sorting_task));

    gettimeofday(&ts_start, NULL);
    srand(ts_start.tv_usec * ts_start.tv_sec);

    pthread_t init_threads[max_tasks];
    int task_indices[max_tasks];
    for (int i = 0; i < max_tasks; i++)
    {
        task_indices[i] = i;
        pthread_create(&init_threads[i], NULL, init_data, &task_indices[i]);
    }

    for (int i = 0; i < max_tasks; i++)
    {
        pthread_join(init_threads[i], NULL);
    }

    pthread_t threads[n_threads];
    for (int i = 0; i < n_threads; i++)
    {
        pthread_create(&(threads[i]), NULL, worker, NULL);
    }

    for (int i = 0; i < max_tasks; i++)
    {
        pthread_mutex_lock(&m);

        tasks[n_tasks].a = data + (n_data / max_tasks) * n_tasks;
        tasks[n_tasks].n_a = n_data / max_tasks;
        if (n_tasks == max_tasks - 1)
            tasks[n_tasks].n_a += n_data % max_tasks;
        tasks[n_tasks].status = UNDONE;

        n_undone++;
        n_tasks++;

        pthread_cond_signal(&cv);
        pthread_mutex_unlock(&m);
    }

    for (int i = 0; i < n_threads; i++)
    {
        pthread_join(threads[i], NULL);
    }

    // 병합 작업을 병렬로 수행
    pthread_t merge_threads[n_threads];
    int merge_task_indices[n_threads];
    for (int i = 0; i < n_threads; i++)
    {
        merge_task_indices[i] = i * 2;
        pthread_create(&merge_threads[i], NULL, merge_worker, &merge_task_indices[i]);
    }

    for (int i = 0; i < n_threads; i++)
    {
        pthread_join(merge_threads[i], NULL);
    }

    // 모든 병합이 완료된 후 단일 스레드에서 마지막 병합 수행
    int n_sorted = n_data / n_tasks;
    for (int i = 1; i < n_tasks; i++)
    {
        if (tasks[i].status != UNDONE)
        {
            merge_lists(data, n_sorted, tasks[i].a, tasks[i].n_a);
            n_sorted += tasks[i].n_a;
        }
    }

    gettimeofday(&ts_end, NULL);
    double elapsed = (ts_end.tv_sec - ts_start.tv_sec) + ((ts_end.tv_usec - ts_start.tv_usec) / 1000000.0);
    printf("Execution time: %f seconds\n", elapsed);

    free(data);
    free(tasks);

    return EXIT_SUCCESS;
}

// merge_lists 함수는 두 개의 정렬된 배열을 하나의 정렬된 배열로 병합합니다.
// a1, a2: 병합할 두 배열의 포인터
// n_a1, n_a2: 각 배열의 요소 수
void merge_lists(double *a1, int n_a1, double *a2, int n_a2) {
    // a_m: 병합된 결과를 임시로 저장할 배열
    double *a_m = (double *)calloc(n_a1 + n_a2, sizeof(double));

    int i = 0; // a_m 배열의 인덱스

    int top_a1 = 0; // a1 배열의 현재 인덱스
    int top_a2 = 0; // a2 배열의 현재 인덱스

    // 두 배열의 모든 요소를 순회하며 병합 작업을 수행
    for (i = 0; i < n_a1 + n_a2; i++) {
        if (top_a2 >= n_a2) { // a2 배열의 모든 요소가 이미 병합되었을 경우
            a_m[i] = a1[top_a1]; // a1의 남은 요소를 a_m에 추가
            top_a1++;
        } else if (top_a1 >= n_a1) { // a1 배열의 모든 요소가 이미 병합되었을 경우
            a_m[i] = a2[top_a2]; // a2의 남은 요소를 a_m에 추가
            top_a2++;
        } else if (a1[top_a1] < a2[top_a2]) { // a1의 현재 요소가 a2의 현재 요소보다 작거나 같은 경우
            a_m[i] = a1[top_a1]; // a1의 요소를 a_m에 추가
            top_a1++;
        } else { // a2의 현재 요소가 a1의 현재 요소보다 작은 경우
            a_m[i] = a2[top_a2]; // a2의 요소를 a_m에 추가
            top_a2++;
        }
    }

    // 병합된 결과를 원래의 a1 배열 위치에 복사
    memcpy(a1, a_m, (n_a1 + n_a2) * sizeof(double));

    // 임시 배열 메모리 해제
    free(a_m);
}

void merge_sort(double *a, int n_a)
{
    if (n_a < 2)
        return;

    double *a1;
    int n_a1;
    double *a2;
    int n_a2;

    a1 = a;
    n_a1 = n_a / 2;

    a2 = a + n_a1;
    n_a2 = n_a - n_a1;

    merge_sort(a1, n_a1);
    merge_sort(a2, n_a2);

    merge_lists(a1, n_a1, a2, n_a2);
}
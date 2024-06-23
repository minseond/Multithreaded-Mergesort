# Multithread Mergesort Program 
이 프로그램은 큰 데이터 배열을 여러 스레드를 사용하여 정렬하는 프로그램입니다. 데이터 배열을 작은 부분으로 나누고, 각 부분을 병렬로 정렬한 후 병합하여 전체 배열을 정렬합니다.

## 기능

- **멀티스레드 정렬**: 여러 스레드를 사용하여 데이터 배열의 다른 부분을 동시에 정렬합니다.
- **데이터 초기화**: 데이터 배열은 랜덤 값으로 초기화됩니다.
- **병합 정렬 알고리즘**: 각 데이터 부분은 병합 정렬 알고리즘을 사용하여 정렬됩니다.
- **병렬 병합**: 정렬된 부분을 병합하여 최종 정렬된 배열을 만듭니다.

## 작동 원리

1. **데이터 초기화 (data_init)**: 
   - 데이터 배열을 여러 부분으로 나누고, 각 부분을 여러 스레드를 사용하여 랜덤 값으로 초기화합니다.

2. **정렬 (merge_sort, worker)**:
   - 데이터 배열의 각 부분을 정렬 스레드에 할당하여 병합 정렬 알고리즘을 사용하여 정렬합니다.
   - 스레드는 동시에 작동하여 할당된 부분을 정렬합니다.

3. **병합(merge_lists)**:
   - 정렬된 부분을 추가적인 스레드를 사용하여 병합합니다.
   - 최종 병합은 단일 스레드에서 수행하여 전체 배열을 정렬합니다.


## 사용법

- **컴파일 및 실행**

```bash
gcc -o pmergesort pmergesort.c -pthread

./pmergesort -d <데이터 요소 수> -t <스레드 수>
./pmergesort -d 30 -t 5

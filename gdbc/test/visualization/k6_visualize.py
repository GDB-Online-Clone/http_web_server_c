import json
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import sys

def parse_k6_summary(file_path):
    """
    K6 summary JSON 파일을 파싱하는 함수
    
    Args:
        file_path (str): 요약 JSON 파일 경로
    Returns:
        dict: 파싱된 메트릭 데이터
    """
    with open(file_path, 'r') as f:
        return json.load(f)

def visualize_k6_results(metrics):
    """
    K6 테스트 결과를 시각화하는 함수
    
    Args:
        metrics (dict): K6 메트릭 데이터
    """
    metrics = metrics['metrics']  # metrics 섹션만 추출
    plt.style.use('default')
    
    # 1. HTTP Request Timing Breakdown
    plt.figure(figsize=(15, 10))
    timing_metrics = {
        'blocked': metrics['http_req_blocked']['avg'],
        'connecting': metrics['http_req_connecting']['avg'],
        'sending': metrics['http_req_sending']['avg'],
        'waiting': metrics['http_req_waiting']['avg'],
        'receiving': metrics['http_req_receiving']['avg'],
        'duration': metrics['http_req_duration']['avg']
    }
    
    plt.subplot(2, 2, 1)
    bars = plt.bar(timing_metrics.keys(), timing_metrics.values(), color='#2196F3')
    plt.xticks(rotation=45)
    plt.title('HTTP Request Timing Breakdown (avg, ms)', pad=20)
    plt.ylabel('Time (ms)')
    plt.grid(True, alpha=0.3)
    
    for bar in bars:
        height = bar.get_height()
        plt.text(bar.get_x() + bar.get_width()/2., height,
                f'{height:.3f}',
                ha='center', va='bottom')

    # 2. Response Time Distribution
    duration_data = {
        'min': metrics['http_req_duration']['min'],
        'med': metrics['http_req_duration']['med'],
        'max': metrics['http_req_duration']['max'],
        'p90': metrics['http_req_duration']['p(90)'],
        'p95': metrics['http_req_duration']['p(95)']
    }
    
    plt.subplot(2, 2, 2)
    bars = plt.bar(duration_data.keys(), duration_data.values(), color='#4CAF50')
    plt.title('Response Time Distribution (ms)', pad=20)
    plt.ylabel('Time (ms)')
    plt.grid(True, alpha=0.3)
    
    for bar in bars:
        height = bar.get_height()
        plt.text(bar.get_x() + bar.get_width()/2., height,
                f'{height:.3f}',
                ha='center', va='bottom')

    # 3. Request Rates
    throughput = {
        'HTTP Requests/s': metrics['http_reqs']['count'] / 30.0,  # 30초 동안의 평균
        'Iterations/s': metrics['iterations']['count'] / 30.0,
        'Data Received (KB/s)': metrics['data_received']['count'] / 1024 / 30.0,
        'Data Sent (KB/s)': metrics['data_sent']['count'] / 1024 / 30.0
    }
    
    plt.subplot(2, 2, 3)
    bars = plt.bar(throughput.keys(), throughput.values(), color='#FF9800')
    plt.xticks(rotation=45)
    plt.title('Throughput Analysis', pad=20)
    plt.ylabel('Rate')
    plt.grid(True, alpha=0.3)
    
    for bar in bars:
        height = bar.get_height()
        plt.text(bar.get_x() + bar.get_width()/2., height,
                f'{height:.2f}',
                ha='center', va='bottom')

    # 4. Success Rate (using checks)
    plt.subplot(2, 2, 4)
    success_rate = (metrics['checks']['passes'] / (metrics['checks']['passes'] + metrics['checks']['fails'])) * 100
    plt.pie([success_rate, 100-success_rate], 
            labels=['Passed', 'Failed'],
            colors=['#4CAF50', '#F44336'],
            autopct='%1.1f%%',
            startangle=90)
    plt.title('Checks Success Rate', pad=20)

    plt.tight_layout(pad=3.0)
    plt.savefig('k6_test_results.png', dpi=300, bbox_inches='tight', facecolor='white')
    plt.close()

    # 5. Iteration Duration Analysis
    plt.figure(figsize=(10, 6))
    iteration_duration = {
        'min': metrics['iteration_duration']['min'],
        'med': metrics['iteration_duration']['med'],
        'max': metrics['iteration_duration']['max'],
        'p90': metrics['iteration_duration']['p(90)'],
        'p95': metrics['iteration_duration']['p(95)']
    }
    
    bars = plt.bar(iteration_duration.keys(), iteration_duration.values(), color='#3F51B5')
    plt.title('Iteration Duration Analysis (ms)', pad=20)
    plt.ylabel('Duration (ms)')
    plt.grid(True, alpha=0.3)
    
    for bar in bars:
        height = bar.get_height()
        plt.text(bar.get_x() + bar.get_width()/2., height,
                f'{height:.2f}',
                ha='center', va='bottom')
    
    plt.tight_layout()
    plt.savefig('iteration_duration.png', dpi=300, bbox_inches='tight', facecolor='white')
    plt.close()

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python3 k6_visualize.py <k6_summary.json>")
        sys.exit(1)
        
    json_file = sys.argv[1]
    try:
        metrics = parse_k6_summary(json_file)
        if metrics:
            visualize_k6_results(metrics)
            print("Visualization complete. Check k6_test_results.png and iteration_duration.png")
        else:
            print("Failed to parse metrics data")
    except Exception as e:
        print(f"Error: {str(e)}")
        sys.exit(1)
import yaml
neg=lambda d,s,t="random":{"features_dir":d,"sampling_weight":s,"penalty_weight":1.0,"truth":False,"truncation_strategy":t,"type":"mmap"}
c={"window_step_ms":10,"train_dir":"trained_models/hey_patch",
 "features":[{"features_dir":"feat_pos","sampling_weight":2.0,"penalty_weight":1.0,"truth":True,"truncation_strategy":"truncate_start","type":"mmap"},
  neg("data/speech",10.0),neg("data/dinner_party",10.0),neg("data/no_speech",5.0),
  neg("feat_hardneg",3.0),
  neg("data/dinner_party_eval",0.0,"split")],
 "training_steps":[10000],"positive_class_weight":[1],"negative_class_weight":[20],"learning_rates":[0.001],
 "batch_size":128,"time_mask_max_size":[0],"time_mask_count":[0],"freq_mask_max_size":[0],"freq_mask_count":[0],
 "eval_step_interval":500,"clip_duration_ms":1500,"target_minimization":0.9,"minimization_metric":None,
 "maximization_metric":"average_viable_recall"}
yaml.dump(c,open("training_parameters.yaml","w"))

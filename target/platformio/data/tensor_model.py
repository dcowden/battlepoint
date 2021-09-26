import numpy as np
import pandas as pd
import tensorflow as tf

from sklearn.model_selection import train_test_split
from tensorflow.keras import layers
from tensorflow.keras.layers.experimental import preprocessing

#only 10 fields supported by microkernel
#FIELDS=['energy','peak_0','peak_1000','peak_2000','peak_3000','peak_4000','overallAvgEnergy','bin0','bin1','bin2','bin3','bin4','bin5','bin6','bin7','bin8',
#    'bin9','bin10','bin11','bin12','bin13','bin14','bin15','bin16','bin17','bin18','bin19']
FIELDS=['energy','peak_0','peak_1000','peak_2000','peak_3000','peak_4000','overallAvgEnergy','bin0','bin1','bin2']
#FIELDS=['peak_0','peak_1000','peak_2000','peak_3000','peak_4000']
#FIELDS=['energy','overallAvgEnergy','bin0','bin1','bin2']

fake_hits_df = pd.read_csv('fake_hits_metal_target.csv') # fake hits only
real_hits_df = pd.read_csv('real_hits_metal_target.csv') #hits only
fake_hits_df = fake_hits_df[FIELDS]
real_hits_df = real_hits_df[FIELDS]
fake_hits_df = fake_hits_df.astype({'peak_0': 'float64','peak_1000': 'float64','peak_2000': 'float64','peak_3000': 'float64','peak_4000': 'float64'})
real_hits_df = real_hits_df.astype({'peak_0': 'float64','peak_1000': 'float64','peak_2000': 'float64','peak_3000': 'float64','peak_4000': 'float64'})
fake_hits_df['target'] = 0
real_hits_df['target'] = 1
all_df = pd.concat([fake_hits_df,real_hits_df])

train_df, test_df = train_test_split(all_df, test_size=0.2)
train_df, val_df = train_test_split(train_df, test_size=0.2)
print(len(train_df), 'train examples')
print(len(val_df), 'validation examples')
print(len(test_df), 'test examples')

print(train_df.dtypes)


# A utility method to create a tf.data dataset from a Pandas Dataframe
def df_to_dataset(dataframe, shuffle=True, batch_size=32):
  dataframe = dataframe.copy()
  labels = dataframe.pop('target')
  ds = tf.data.Dataset.from_tensor_slices((dict(dataframe), labels))
  if shuffle:
    ds = ds.shuffle(buffer_size=len(dataframe))
  ds = ds.batch(batch_size)
  ds = ds.prefetch(batch_size)
  return ds

def get_normalization_layer(name, dataset):
  # Create a Normalization layer for our feature.
  normalizer = preprocessing.Normalization(axis=None)

  # Prepare a Dataset that only yields our feature.
  feature_ds = dataset.map(lambda x, y: x[name])

  # Learn the statistics of the data.
  normalizer.adapt(feature_ds)

  return normalizer
  
BATCH_SIZE=25
train_ds = df_to_dataset(train_df,batch_size=BATCH_SIZE)
val_ds = df_to_dataset(val_df,batch_size=BATCH_SIZE)
test_ds = df_to_dataset(test_df,batch_size=BATCH_SIZE)


all_inputs=[]
encoded_features = []
for h in FIELDS:
    numeric_col = tf.keras.Input(shape=(1,),name=h)
    normalization_layer = get_normalization_layer(h,train_ds)
    encoded_numeric_col = normalization_layer(numeric_col)
    all_inputs.append(numeric_col)
    encoded_features.append(encoded_numeric_col)


all_features = tf.keras.layers.concatenate(encoded_features)
x = tf.keras.layers.Dense(16, activation="relu")(all_features)
x = tf.keras.layers.Dropout(0.5)(x)
output = tf.keras.layers.Dense(1)(x)
model = tf.keras.Model(all_inputs, output)
model.compile(optimizer='adam',
              loss=tf.keras.losses.BinaryCrossentropy(from_logits=True),
              metrics=["accuracy"])


model.fit(train_ds, epochs=200, validation_data=val_ds)

loss, accuracy = model.evaluate(test_ds)
print("Accuracy", accuracy)

model.save('target_hit_classifier')
reloaded_model = tf.keras.models.load_model('target_hit_classifier')
model.summary()

#from tinymlgen import port
#c_code = port(reloaded_model, pretty_print=True)
#print(c_code)

converter = tf.lite.TFLiteConverter.from_keras_model(model)
converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS]
#converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]
#converter.inference_input_type = tf.int8  # or tf.uint8
#converter.inference_output_type = tf.int8  # or tf.uint8
#converter.target_spec.supported_types = [tf.float16]
#converter.optimizations = [tf.lite.Optimize.OPTIMIZE_FOR_SIZE]
#converter.optimizations = [tf.lite.Optimize.DEFAULT]
tflite_model = converter.convert()

# Save the model to disk
model_size = open("target_classifier_quantized.tflite", "wb").write(tflite_model)
print("Output Model is %d bytes" % model_size)


input_df = pd.read_csv('input_to_predict.csv')
#input_df = input_df.drop(['sample_no', 'side', 'hits', 'totalSampleTime[ms]', 'avgSampleTime[ms]',  'Unnamed: 32'],axis=1)
input_df = input_df [FIELDS]


# THIS IS DUMB, but for now i know how to do it this way
# the tutorial here https://www.tensorflow.org/tutorials/structured_data/preprocessing_layers
for counter,r in enumerate(input_df.to_dict(orient='records')):

    j = {name: tf.convert_to_tensor([value]) for name, value in r.items()}
    predictions = reloaded_model.predict(j)
    prob = tf.nn.sigmoid(predictions)

    print("Probability %d of Hit: %0.1f" % (counter,prob*100))  

print ("Now Run xxd -i target_classifier_quantized.tflite > ../lib/battlepoint/TargetClassifier.h")
